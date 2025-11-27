# mod_logfile_domain

FreeSWITCH module for domain-specific channel logging. Captures all channel-related log messages and writes them to domain-based log files for easier troubleshooting and auditing.

## Features

- **Domain-Specific Logging**: Separate log files per domain (e.g., `domain_example.com.log`)
- **Thread-Safe**: Per-file mutex synchronization with no global contention
- **High Performance**: Hash-based domain caching (O(1) lookup, max 256 domains)
- **Automatic File Management**: Log files created on-demand, rotatable via HUP
- **FreeSWITCH Native**: Uses only FreeSWITCH core APIs (switch_file_t, switch_hash_t, switch_mutex_t)
- **Production Ready**: Follows mod_logfile patterns, zero external dependencies

## Installation

### Prerequisites

- FreeSWITCH 1.8 or higher with development headers
- GNU Autotools (autoconf, automake, libtool) or CMake
- GCC/Clang compiler

### Build with Automake (Recommended)

```bash
cd mod_logfile_domain
autoreconf -i
./configure --with-freeswitch=/usr
make
sudo make install
```

### Build with CMake (Alternative)

```bash
cd mod_logfile_domain
mkdir build && cd build
cmake .. -DFREESWITCH_PREFIX=/usr
make
sudo make install
```

### Install Configuration

```bash
sudo cp mod_logfile_domain/conf/autoload_configs/logfile_domain.conf.xml \
  /etc/freeswitch/autoload_configs/
```

### Enable Module

Edit `/etc/freeswitch/autoload_configs/modules.conf.xml`:
```xml
<load module="mod_logfile_domain"/>
```

Restart FreeSWITCH:
```bash
sudo systemctl restart freeswitch
```

## Configuration

Configuration file: `/etc/freeswitch/autoload_configs/logfile_domain.conf.xml`

```xml
<configuration name="logfile_domain.conf" description="Domain-specific File Logging">
  <settings>
    <!-- Auto rotate on HUP (default: true) -->
    <param name="rotate-on-hup" value="true"/>
  </settings>
  <profiles>
    <profile name="default">
      <settings>
        <!-- Log rotation size in bytes (default: 10MB) -->
        <param name="rollover" value="10485760"/>
        <!-- Max log files before wrapping (optional) -->
        <!-- <param name="maximum-rotate" value="32"/> -->
      </settings>
      <mappings>
        <!-- Log all levels -->
        <map name="all" value="debug,info,notice,warning,err,crit,alert"/>
      </mappings>
    </profile>
  </profiles>
</configuration>
```

## Usage

### Set Domain in Dialplan

#### Dialplan XML
```xml
<action application="set" data="domain_name=example.com"/>
```

#### With Condition
```xml
<condition field="destination_number" expression="^1001$">
  <action application="set" data="domain_name=sales.example.com"/>
  <action application="bridge" data="sofia/internal/1001@example.com"/>
</condition>
```

### Verify Domain Logging

```bash
# Check module is loaded
fs_cli -x "load mod_logfile_domain"

# View domain logs
tail -f /var/log/freeswitch/domain_example.com.log

# Multiple domains
tail -f /var/log/freeswitch/domain_*.log
```

### Lua Example

```lua
session:execute("set", "domain_name=lua.example.com")
session:execute("bridge", "sofia/internal/1001@example.com")
```

### JavaScript Example

```javascript
session.execute("set", "domain_name=javascript.example.com");
session.execute("bridge", "sofia/internal/1001@example.com");
```

## Architecture

### Module Components

```
mod_logfile_domain.c (374 lines)
├── domain_cache_entry_t        (Per-domain file handle + mutex)
├── get_domain_entry()          (Hash-based cache lookup)
├── open_domain_logfile()       (switch_file_t operations)
├── write_domain_log()          (Thread-safe write with mutex)
├── extract_domain()            (Get domain_name variable)
├── mod_logfile_domain_logger() (Main logging hook)
└── Module lifecycle            (Load/shutdown with cleanup)
```

### Cache Strategy

- **Type**: Hash table (switch_hash_t)
- **Lookup**: O(1) average
- **Max Domains**: 256
- **Memory per Entry**: ~640 bytes
- **Max Memory**: ~160 KB
- **Synchronization**: Per-file switch_mutex_t (no global lock)

### Log File Naming

```
/var/log/freeswitch/domain_<domain_name>.log
```

Examples:
- `domain_example.com.log`
- `domain_sales.example.com.log`
- `domain_support.example.com.log`

## Performance

| Metric | Value |
|--------|-------|
| Code Size | 374 lines |
| Memory per Domain | ~640 bytes |
| Max Domains | 256 |
| Max Total Memory | ~160 KB |
| Thread Safety | Yes (per-file mutexes) |
| External Dependencies | None (FreeSWITCH only) |

## Troubleshooting

### Module Not Loading

Check FreeSWITCH logs:
```bash
tail -f /var/log/freeswitch/freeswitch.log | grep -i logfile_domain
```

Verify module is compiled:
```bash
ls /usr/lib/freeswitch/mod/mod_logfile_domain.so
```

Ensure configuration exists:
```bash
ls /etc/freeswitch/autoload_configs/logfile_domain.conf.xml
```

### No Domain Logs Created

1. Verify domain_name is set in dialplan:
   ```xml
   <action application="set" data="domain_name=example.com"/>
   ```

2. Check FreeSWITCH has write permissions to `/var/log/freeswitch/`

3. Enable debug logging:
   ```bash
   fs_cli -x "log_level debug"
   ```

4. Monitor in real-time:
   ```bash
   tail -f /var/log/freeswitch/freeswitch.log
   ```

### Log Files Not Rotating

1. Verify rollover size in config (default: 10MB)
2. Test rotation manually:
   ```bash
   # Send HUP signal to freeswitch
   ps aux | grep freeswitch | grep -v grep | awk '{print $2}' | xargs kill -HUP
   ```

## Directory Structure

```
mod_logfile_domain/
├── mod_logfile_domain.c              (Main module - 374 lines)
├── Makefile.am                       (Automake configuration)
├── CMakeLists.txt                    (CMake configuration)
├── .gitkeep                          (Git directory marker)
└── conf/
    └── autoload_configs/
        └── logfile_domain.conf.xml   (Module configuration)
```

## Build System

### Automake (Production)

Uses FreeSWITCH's standard `modmake.rulesam`:
```makefile
include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_logfile_domain

mod_LTLIBRARIES = mod_logfile_domain.la
mod_logfile_domain_la_SOURCES  = mod_logfile_domain.c
mod_logfile_domain_la_CFLAGS   = $(AM_CFLAGS)
mod_logfile_domain_la_LIBADD   = $(switch_builddir)/libfreeswitch.la
mod_logfile_domain_la_LDFLAGS  = -avoid-version -module -no-undefined -shared
```

### CMake (Alternative)

Supports standard FreeSWITCH installation paths via pkg-config.

## Testing

### Basic Test

```bash
# 1. Start FreeSWITCH
sudo systemctl start freeswitch

# 2. Verify module loaded
fs_cli -x "load mod_logfile_domain"

# 3. Make test call with domain set in dialplan
fs_cli -x "originate sofia/internal/1001@example.com &bridge(sofia/internal/1002@example.com)"

# 4. Verify domain log created
tail /var/log/freeswitch/domain_*.log
```

### Concurrent Domain Test

```bash
# Multiple parallel calls to different domains
for domain in sales support billing; do
  fs_cli -x "originate sofia/internal/user@$domain.com &bridge(sofia/internal/1001@example.com)" &
done
```

## FAQ

**Q: Can I log multiple domains in one file?**  
A: No, this module is designed for per-domain isolation. Use mod_logfile for multi-sink logging.

**Q: Does setting domain_name affect call routing?**  
A: No, it only affects logging. Routing is controlled by bridge destinations.

**Q: What's the performance overhead?**  
A: Minimal - O(1) hash lookups, per-file mutex (no global lock), memory-pooled.

**Q: Can I rotate logs manually?**  
A: Yes, send HUP signal: `kill -HUP $(pgrep freeswitch)`

**Q: Does it work with clustering?**  
A: Each FreeSWITCH instance logs locally. Aggregate externally if needed.

## GitHub Deployment

Ready for GitHub:
- `.gitignore` configured for clean repository
- All build artifacts excluded
- Only production files tracked
- Follow FreeSWITCH conventions

### Quick Clone & Build

```bash
git clone https://github.com/YOUR_USERNAME/mod_logfile_domain.git
cd mod_logfile_domain
cd mod_logfile_domain
autoreconf -i && ./configure --with-freeswitch=/usr && make
sudo make install
```

## Version

**Version**: 1.0  
**Status**: Production Ready  
**Compatibility**: FreeSWITCH 1.8+  
**License**: MPL-2.0  
**Last Updated**: November 27, 2025

## Support & Contributing

- **Issues**: Report bugs on GitHub
- **Questions**: Check FreeSWITCH community forums
- **Contributing**: Pull requests welcome
- **Pattern**: Based on mod_logfile best practices
