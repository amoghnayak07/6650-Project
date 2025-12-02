# Quick Start Guide - Persistence & Load Balancer

## Prerequisites

- C++11 compiler (g++)
- POSIX-compliant OS (Linux/MacOS)
- Make utility
- Standard UNIX tools (netstat, sleep, pkill optional)

## Build

```bash
cd "PA3 directory"
make all          # Builds: server, client, loadbalancer
```

## Quick Demo Setup (3 Factories + Load Balancer)

### Terminal 1 - Start Primary Factory

```bash
./server 8000 0 2 1 127.0.0.1 8001 2 127.0.0.1 8002
```

- Listening on port 8000
- Factory ID: 0 (Primary)
- Connected to factories 1 and 2

### Terminal 2 - Start Backup Factory 1

```bash
./server 8001 1 2 0 127.0.0.1 8000 2 127.0.0.1 8002
```

- Listening on port 8001
- Factory ID: 1 (Backup)
- Connected to factories 0 and 2

### Terminal 3 - Start Backup Factory 2

```bash
./server 8002 2 2 0 127.0.0.1 8000 1 127.0.0.1 8001
```

- Listening on port 8002
- Factory ID: 2 (Backup)
- Connected to factories 0 and 1

### Terminal 4 - Start Load Balancer

```bash
./loadbalancer 9000 9001 3 \
    0 127.0.0.1 8000 \
    1 127.0.0.1 8001 \
    2 127.0.0.1 8002
```

- Write requests: Port 9000 (routes to factory 0)
- Read requests: Port 9001 (round-robin across all)

### Terminal 5 - Run Client Through Load Balancer

```bash
./client 127.0.0.1 9000
```

## Persistence in Action

### Observe WAL Files

After running operations, check for WAL files:

```bash
ls -la wal_factory_*.wal
```

You'll see:

- `wal_factory_0.wal` - Primary factory log
- `wal_factory_1.wal` - Backup factory log
- `wal_factory_2.wal` - Backup factory log

### Test Crash Recovery

1. **Perform some operations** with the client
2. **Kill primary factory** in Terminal 1:
   ```bash
   # In another terminal or Ctrl+C in Terminal 1
   kill -9 <PID>
   ```
3. **Check WAL file size**:
   ```bash
   ls -l wal_factory_0.wal
   ```
4. **Restart the primary**:
   ```bash
   ./server 8000 0 2 1 127.0.0.1 8001 2 127.0.0.1 8002
   ```
5. **Observe recovery**:
   - Should print: "Factory 0 - Recovering from WAL with X entries..."
   - Should print: "Factory 0 - Recovery complete. Last index: X"
   - State is restored from disk!

## Load Balancer Testing

### Test Write Routing (Primary-Only)

```bash
# Connect to write port (9000)
./client 127.0.0.1 9000
# All requests go ONLY to factory 0
```

### Test Read Routing (Round-Robin)

```bash
# In multiple terminals, connect to read port (9001)
./client 127.0.0.1 9001  # First connection → factory 0
./client 127.0.0.1 9001  # Second connection → factory 1
./client 127.0.0.1 9001  # Third connection → factory 2
./client 127.0.0.1 9001  # Fourth connection → factory 0 (repeats)
```

## Monitoring

### Check Factory Connections

```bash
netstat -an | grep 800  # See factory ports
netstat -an | grep 900  # See LB ports
```

### View WAL File Details

```bash
# Check file size (16 bytes per entry, so size/16 = number of entries)
stat wal_factory_0.wal

# Compare before/after recovery
wc -c wal_factory_0.wal
```

### Monitor LB Activity

Check stdout output of load balancer process for routing decisions:

- "Write request routed to primary factory 0"
- "Read request routed to factory X (round-robin)"

## Troubleshooting

### Load Balancer Won't Start

- Check if ports 9000, 9001 are already in use
- Kill previous instances: `pkill -f loadbalancer`
- Try different ports if needed

### Factory Won't Connect

- Ensure peer IPs and ports are correct
- Check firewall settings
- Try `127.0.0.1` instead of `localhost`
- Verify all factories started before load balancer

### No WAL Files Created

- Check current directory permissions
- Files appear after first write operation
- Verify factory accepts client connections

### Recovery Not Working

- Check WAL file exists: `ls wal_factory_*.wal`
- Ensure file permissions allow reading
- Check disk space is available
- Look for error messages in terminal output

## Advanced: Custom Configuration

### Using Different Ports

```bash
# Factories on different ports
./server 7000 0 2 1 127.0.0.1 7001 2 127.0.0.1 7002
./server 7001 1 2 0 127.0.0.1 7000 2 127.0.0.1 7002
./server 7002 2 2 0 127.0.0.1 7000 1 127.0.0.1 7001

# LB on different ports
./loadbalancer 8000 8001 3 \
    0 127.0.0.1 7000 \
    1 127.0.0.1 7001 \
    2 127.0.0.1 7002

# Client connects to new LB ports
./client 127.0.0.1 8000
```

### Multi-Machine Setup

```bash
# On Machine 1 (factory 0)
./server 8000 0 2 1 192.168.1.2 8001 2 192.168.1.3 8002

# On Machine 2 (factory 1)
./server 8001 1 2 0 192.168.1.1 8000 2 192.168.1.3 8002

# On Machine 3 (factory 2)
./server 8002 2 2 0 192.168.1.1 8000 1 192.168.1.2 8001

# Load Balancer on any machine
./loadbalancer 9000 9001 3 \
    0 192.168.1.1 8000 \
    1 192.168.1.2 8001 \
    2 192.168.1.3 8002
```

## Performance Tips

1. **Optimize WAL**: Run factories on SSD for faster persistence
2. **Network**: Use local connections (127.0.0.1) for testing
3. **Concurrency**: LB handles multiple clients well, use threading in clients for load testing
4. **Recovery**: For large WAL files, recovery may take several seconds

## Cleanup

```bash
# Stop all processes
pkill -f server
pkill -f loadbalancer

# Remove WAL files
rm wal_factory_*.wal

# Clean build artifacts
make clean
```

## Key Concepts

### Persistence

- Every write operation is appended to a WAL file before being replicated
- On crash, the factory restarts and recovers all operations from the WAL
- Guarantees durability even if factory process crashes

### Load Balancer

- **Write Port**: Always forwards to primary factory (id=0) for consistency
- **Read Port**: Distributes read requests across all factories using round-robin
- **TCP Proxy**: Transparent proxying - client sees factory, factory sees client
- **Connection**: Independent per connection, closed by either side

## Documentation

See detailed documentation in:

- `PERSISTENCE_LOADBALANCER_GUIDE.md` - Complete technical guide
- `IMPLEMENTATION_SUMMARY.md` - Implementation details
