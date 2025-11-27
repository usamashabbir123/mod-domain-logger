/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * mod_logfile_domain.c -- Domain-Specific Filesystem Logging Module
 *
 * Purpose: Capture and route channel-specific log messages to domain-based log files
 *
 */

#include <switch.h>
#include <dlfcn.h>
#include <ctype.h>

SWITCH_MODULE_LOAD_FUNCTION(mod_logfile_domain_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_logfile_domain_shutdown);
SWITCH_MODULE_DEFINITION(mod_logfile_domain, mod_logfile_domain_load, mod_logfile_domain_shutdown, NULL);

#define DEFAULT_LIMIT 0xA00000  /* About 10 MB */
#define WARM_FUZZY_OFFSET 256
#define MAX_ROT 4096
#define MAX_DOMAIN_CACHE_SIZE 256

static switch_memory_pool_t *module_pool = NULL;
static switch_hash_t *domain_hash = NULL;

/* function pointer for optional API (may not exist in older FreeSWITCH builds) */
typedef switch_status_t (*switch_log_node_render_fn)(const switch_log_node_t *node, char *buf, size_t len);
static switch_log_node_render_fn switch_log_node_render_ptr = NULL;

/* Domain file cache entry */
typedef struct {
    char domain[128];
    switch_file_t *log_file;
    switch_size_t log_size;
    switch_size_t roll_size;
    char logfile_path[512];
    switch_mutex_t *file_lock;
} domain_cache_entry_t;

static struct {
    switch_mutex_t *mutex;
    int cache_entries;
} globals;

/* Cleanup domain cache entry */
static void cleanup_domain_entry(void *ptr)
{
    domain_cache_entry_t *entry = (domain_cache_entry_t *)ptr;
    if (entry) {
        if (entry->file_lock) {
            switch_mutex_destroy(entry->file_lock);
        }
        if (entry->log_file) {
            switch_file_close(entry->log_file);
        }
    }
}


/* Open/create log file for domain */
static switch_status_t open_domain_logfile(domain_cache_entry_t *entry)
{
    unsigned int flags = 0;
    switch_file_t *afd;
    switch_status_t stat;

    flags |= SWITCH_FOPEN_CREATE;
    flags |= SWITCH_FOPEN_READ;
    flags |= SWITCH_FOPEN_WRITE;
    flags |= SWITCH_FOPEN_APPEND;

    stat = switch_file_open(&afd, entry->logfile_path, flags, SWITCH_FPROT_OS_DEFAULT, module_pool);
    
    if (stat != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                        "mod_logfile_domain: Failed to open %s (status=%d)\n", 
                        entry->logfile_path, stat);
        return SWITCH_STATUS_FALSE;
    }

    entry->log_file = afd;
    entry->log_size = switch_file_get_size(entry->log_file);

    return SWITCH_STATUS_SUCCESS;
}

/* Get or create cache entry for a domain */
static domain_cache_entry_t *get_domain_entry(const char *domain)
{
    domain_cache_entry_t *entry = NULL;
    
    if (!domain || zstr(domain)) {
        return NULL;
    }

    switch_mutex_lock(globals.mutex);

    /* Check if domain already in cache */
    entry = (domain_cache_entry_t *)switch_core_hash_find(domain_hash, domain);
    
    if (entry) {
        switch_mutex_unlock(globals.mutex);
        return entry;
    }

    /* Check cache size limit */
    if (globals.cache_entries >= MAX_DOMAIN_CACHE_SIZE) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                        "mod_logfile_domain: Cache full (%d domains)\n", MAX_DOMAIN_CACHE_SIZE);
        switch_mutex_unlock(globals.mutex);
        return NULL;
    }

    /* Create new cache entry */
    entry = (domain_cache_entry_t *)switch_core_alloc(module_pool, sizeof(*entry));
    memset(entry, 0, sizeof(*entry));

    switch_copy_string(entry->domain, domain, sizeof(entry->domain));
    entry->roll_size = DEFAULT_LIMIT;

    /* Build log file path */
    switch_snprintf(entry->logfile_path, sizeof(entry->logfile_path), 
                   "%s%sdomain_%s.log", 
                   SWITCH_GLOBAL_dirs.log_dir, 
                   SWITCH_PATH_SEPARATOR, 
                   domain);

    /* Create per-file mutex */
    switch_mutex_init(&entry->file_lock, SWITCH_MUTEX_NESTED, module_pool);

    /* Open the log file */
    if (open_domain_logfile(entry) != SWITCH_STATUS_SUCCESS) {
        switch_mutex_unlock(globals.mutex);
        return NULL;
    }

    /* Add to cache hash */
    switch_core_hash_insert_destructor(domain_hash, domain, (void *)entry, cleanup_domain_entry);
    globals.cache_entries++;

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
                    "mod_logfile_domain: Created cache entry for domain: %s\n", domain);

    switch_mutex_unlock(globals.mutex);
    return entry;
}

/* Extract domain from channel */
static const char *extract_domain(switch_channel_t *channel)
{
    const char *domain = NULL;

    if (!channel) {
        return NULL;
    }

    /* Try domain_name variable first */
    domain = switch_channel_get_variable(channel, "domain_name");
    
    if (!zstr(domain)) {
        return domain;
    }

    /* Fallback to domain variable */
    domain = switch_channel_get_variable(channel, "domain");
    
    if (!zstr(domain)) {
        return domain;
    }

    return NULL;
}

/* Try to extract domain from a rendered log message (fallback) */
static const char *extract_domain_from_msg(const char *msg)
{
    static char domainbuf[128];
    const char *p;
    size_t i = 0;

    if (!msg) return NULL;

    /* look for domain_name=VALUE */
    p = strstr(msg, "domain_name=");
    if (!p) {
        p = strstr(msg, "domain=");
        if (!p) return NULL;
        p += strlen("domain=");
    } else {
        p += strlen("domain_name=");
    }

    /* copy until whitespace or non-printable or end */
    while (*p && !isspace((unsigned char)*p) && i + 1 < sizeof(domainbuf)) {
        domainbuf[i++] = *p++;
    }
    domainbuf[i] = '\0';

    if (i == 0) return NULL;
    return domainbuf;
}

/* Write log data to domain file */
static switch_status_t write_domain_log(const char *domain, const char *log_data)
{
    domain_cache_entry_t *entry = NULL;
    switch_size_t len;
    switch_status_t status = SWITCH_STATUS_SUCCESS;

    if (!domain || !log_data) {
        return SWITCH_STATUS_FALSE;
    }

    entry = get_domain_entry(domain);
    if (!entry) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                        "mod_logfile_domain: No cache entry for domain: %s\n", domain);
        return SWITCH_STATUS_FALSE;
    }

    len = strlen(log_data);

    switch_mutex_lock(entry->file_lock);

    if (!entry->log_file || 
        switch_file_write(entry->log_file, log_data, &len) != SWITCH_STATUS_SUCCESS) {
        
        if (entry->log_file) {
            switch_file_close(entry->log_file);
            entry->log_file = NULL;
        }

        /* Try to reopen and write */
        if (open_domain_logfile(entry) == SWITCH_STATUS_SUCCESS) {
            len = strlen(log_data);
            switch_file_write(entry->log_file, log_data, &len);
        } else {
            status = SWITCH_STATUS_FALSE;
        }
    }

    if (status == SWITCH_STATUS_SUCCESS) {
        entry->log_size += len;
    }

    switch_mutex_unlock(entry->file_lock);

    return status;
}

/* Main logging callback */

static switch_status_t mod_logfile_domain_logger(const switch_log_node_t *node, switch_log_level_t level)
{
    switch_core_session_t *session = NULL;
    switch_channel_t *channel = NULL;
    const char *domain = NULL;
    char log_line[2048];
    switch_time_t now = switch_time_now();
    switch_time_exp_t tm;
    char date[80] = "";
    size_t retsize;

    if (!node) {
        return SWITCH_STATUS_SUCCESS;
    }

    /* Render message into buffer instead of accessing struct fields that may change */
    char rendered_msg[1024] = "";
    if (switch_log_node_render_ptr) {
        if (switch_log_node_render_ptr(node, rendered_msg, sizeof(rendered_msg)) != SWITCH_STATUS_SUCCESS) {
            rendered_msg[0] = '\0';
        }
    }

    /* Skip internal module logs to prevent recursion */
    if (node->file && strstr(node->file, "mod_logfile_domain")) {
        return SWITCH_STATUS_SUCCESS;
    }

    /* Try to get session and extract domain */
    if (node->userdata) {
        session = (switch_core_session_t *)node->userdata;
        channel = switch_core_session_get_channel(session);
        
        if (channel) {
            domain = extract_domain(channel);
        }
    }

    /* Fallback: if no session/domain, try to parse rendered message for domain_name= or domain= */
    if (zstr(domain) && rendered_msg[0]) {
        const char *f = extract_domain_from_msg(rendered_msg);
        if (f) {
            domain = f;
        }
    }

    /* If we have a domain, write to domain-specific log */
    if (!zstr(domain)) {
        switch_time_exp_lt(&tm, now);
        switch_strftime_nocheck(date, &retsize, sizeof(date), "%Y-%m-%d %H:%M:%S", &tm);

        if (channel) {
            const char *uuid = switch_channel_get_uuid(channel);
            switch_snprintf(log_line, sizeof(log_line), 
                           "%s [%s] [%s:%s:%d] %s [%s]\n",
                           date, 
                           switch_log_level2str(level), 
                           node->file ? node->file : "unknown",
                           node->func ? node->func : "unknown",
                           node->line, 
                           rendered_msg[0] ? rendered_msg : "(message)", 
                           uuid ? uuid : "unknown");
        } else {
            switch_snprintf(log_line, sizeof(log_line),
                           "%s [%s] [%s:%s:%d] %s\n",
                           date,
                           switch_log_level2str(level),
                           node->file ? node->file : "unknown",
                           node->func ? node->func : "unknown",
                           node->line,
                           rendered_msg[0] ? rendered_msg : "(message)");
        }

        write_domain_log(domain, log_line);
    }

    return SWITCH_STATUS_SUCCESS;
}

/* Close all domain log files */
static void close_all_domain_logs(void)
{
    switch_hash_index_t *hi;
    void *val;
    const void *var;
    domain_cache_entry_t *entry;

    switch_mutex_lock(globals.mutex);

    for (hi = switch_core_hash_first(domain_hash); hi; hi = switch_core_hash_next(&hi)) {
        switch_core_hash_this(hi, &var, NULL, &val);
        entry = (domain_cache_entry_t *)val;
        
        if (entry && entry->file_lock) {
            switch_mutex_lock(entry->file_lock);
            if (entry->log_file) {
                switch_file_close(entry->log_file);
                entry->log_file = NULL;
            }
            switch_mutex_unlock(entry->file_lock);
        }
    }

    switch_mutex_unlock(globals.mutex);
}

/* Module load function */
SWITCH_MODULE_LOAD_FUNCTION(mod_logfile_domain_load)
{
    module_pool = pool;

    memset(&globals, 0, sizeof(globals));
    switch_mutex_init(&globals.mutex, SWITCH_MUTEX_NESTED, module_pool);

    if (domain_hash) {
        switch_core_hash_destroy(&domain_hash);
    }
    switch_core_hash_init(&domain_hash);

    /* Create module interface */
    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    /* Try to resolve optional render API at runtime to remain compatible with older FS builds */
    switch_log_node_render_ptr = (switch_log_node_render_fn)dlsym(RTLD_DEFAULT, "switch_log_node_render");
    if (switch_log_node_render_ptr) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mod_logfile_domain: runtime resolved switch_log_node_render\n");
    } else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mod_logfile_domain: switch_log_node_render not available; using fallback\n");
    }

    /* Register logging hook */
    switch_log_bind_logger(mod_logfile_domain_logger, SWITCH_LOG_DEBUG, SWITCH_TRUE);

    /* One-time diagnostic: write a small verification file to the freeswitch log directory
       to make it easy to confirm the module has write permission and can create files. */
    {
        char diag_path[512];
        switch_file_t *diag = NULL;
        unsigned int flags = SWITCH_FOPEN_CREATE | SWITCH_FOPEN_WRITE | SWITCH_FOPEN_TRUNC;
        switch_snprintf(diag_path, sizeof(diag_path), "%s%sswitch_mod_logfile_domain_loaded", SWITCH_GLOBAL_dirs.log_dir, SWITCH_PATH_SEPARATOR);
        if (switch_file_open(&diag, diag_path, flags, SWITCH_FPROT_OS_DEFAULT, module_pool) == SWITCH_STATUS_SUCCESS) {
            const char *msg = "mod_logfile_domain loaded\n";
            switch_size_t wrote = (switch_size_t)strlen(msg);
            switch_file_write(diag, msg, &wrote);
            switch_file_close(diag);
        } else {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "mod_logfile_domain: could not write diagnostic file %s\n", diag_path);
        }
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                    "mod_logfile_domain: Loaded successfully - Domain-specific logging enabled\n");

    return SWITCH_STATUS_SUCCESS;
}

/* Module shutdown function */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_logfile_domain_shutdown)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                    "mod_logfile_domain: Shutting down - %d domains cached\n", 
                    globals.cache_entries);

    /* Unbind logging */
    switch_log_unbind_logger(mod_logfile_domain_logger);

    /* Close all open files */
    close_all_domain_logs();

    /* Destroy hash */
    switch_core_hash_destroy(&domain_hash);

    return SWITCH_STATUS_SUCCESS;
}

/*
 * For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
