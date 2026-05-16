# Quick Start Guide - Production Log Monitoring Demo

## Running the Demo

### Option 1: Simple Direct Run (Recommended - Shows Real-Time Output)
```powershell
# From project root
cd G:\AutomonScript\ams-lang
./examples/logfile_demo/run_demo_simple.ps1

# Or from demo directory
cd examples/logfile_demo
./run_demo_simple.ps1
```
**This shows real-time output as the demo runs!**

### Option 2: From Project Root (Manual)
```powershell
cd G:\AutomonScript\ams-lang
ams build examples/logfile_demo.ams
./logfile_demo.exe
```

### Option 3: Using Quick Run Script
```powershell
# From anywhere in the project
./examples/logfile_demo/quick_run.ps1

# Or from the demo directory
cd examples/logfile_demo
./quick_run.ps1
```

### Option 4: Using Full Demo Script with Simulation
```powershell
# From anywhere in the project
./examples/logfile_demo/run_demo.ps1

# Or from the demo directory
cd examples/logfile_demo
./run_demo.ps1
```
**Note**: This version runs in background and may not show real-time output.

## What You'll See

The demo will:
1. ✅ Open 3 log files (app.log, system.log, security.log)
2. ✅ Start 3 monitoring sources checking at different intervals
3. ✅ Detect errors and security events in the logs
4. ✅ Trigger event handlers when conditions are met
5. ✅ Execute observer responses

### Sample Output
```
============================================================================
PRODUCTION LOG MONITORING SYSTEM
============================================================================
[INIT] LOG FILES OPENED SUCCESSFULLY

--- [SECURITYLOGMONITOR] CHECK #1 ---
[SECURITYLOGMONITOR] FAILED LOGINS: 0
[SECURITYLOGMONITOR] CRITICAL SECURITY EVENT!
>>> [SECURITYTHREATHANDLER] EVENT TRIGGERED <<<
>>> FAILED LOGINS: 0
>>> SECURITY BREACH: 1
>>> THREAT SCORE: 20
    [SECURITYRESPONSE] INITIATING RESPONSE

--- [APPLOGMONITOR] CHECK #1 ---
[APPLOGMONITOR] TOTAL ERRORS: 7
[APPLOGMONITOR] CRITICAL ERRORS: 2
[APPLOGMONITOR] ALERT CONDITION MET
>>> [CRITICALERRORHANDLER] EVENT TRIGGERED <<<
>>> TOTAL ERRORS: 7
>>> CRITICAL ERRORS: 2
>>> PRIORITY: 10
    [ALERTNOTIFIER] SENDING ALERT
    [ERRORLOGGER] LOGGING TO DATABASE

--- [SYSTEMLOGMONITOR] CHECK #1 ---
[SYSTEMLOGMONITOR] HEALTH SCORE: 90
```

## Stopping the Demo

Press **Ctrl+C** to stop the demo at any time.

## Troubleshooting

### Error: "Failed to open LOG_SOURCE: File not found"
**Solution**: Make sure you're running from the project root directory:
```powershell
cd G:\AutomonScript\ams-lang
./logfile_demo.exe
```

### Error: "AMS compiler not found"
**Solution**: Build the compiler first:
```powershell
cmake --build build --config Release
```

### Error: "Could not open file"
**Solution**: Remove any trailing backslashes from the path:
```powershell
# ❌ Wrong
ams build .\examples\logfile_demo.ams\

# ✅ Correct
ams build examples/logfile_demo.ams
```

## Files

- **logfile_demo.ams** - Main AMS program (500+ lines)
- **app.log** - Application logs (40 entries)
- **system.log** - System logs (27 entries)
- **security.log** - Security logs (38 entries)
- **run_demo.ps1** - Full demo with log simulation
- **quick_run.ps1** - Quick test without simulation
- **README.md** - Comprehensive documentation
- **VALIDATION_REPORT.md** - Validation results
- **QUICKSTART.md** - This file

## Next Steps

1. ✅ Run the demo and observe the output
2. ✅ Read the comprehensive README.md for details
3. ✅ Check VALIDATION_REPORT.md for known issues
4. ✅ Modify the demo to experiment with AMS features
5. ✅ Use as a template for your own AMS applications

---

**Need Help?** Check the main README.md or VALIDATION_REPORT.md for detailed information.
