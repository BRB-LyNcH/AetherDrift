# AetherDrift: Deterministic Event-Driven Algorithmic Trading Core

A modular, low-latency algorithmic trading platform built to evaluate strategy patterns and enforce hard risk constraints. The software runtime architecture cleanly decouples event telemetry ingestion, decision/planning strategy compilation, and target execution logic—mirroring structural patterns found in safety-critical autonomous systems.

## Key Features
- **Deterministic Event Orchestration (C++):** Built using an Object-Oriented design that processes sequential market data stream states via strict abstract interfaces.
- **Localized Risk Guardrails:** Features a validation layer targeting account state protections, intercepting invalid order sizes or deficit balances before execution.
- **Vectorized Backtesting Pipeline (Python):** Utilizes Pandas and NumPy arrays to efficiently evaluate quantitative yield and maximum strategy drawdown profiles over historic time series data.

## Directory Structure
```text
AetherDrift/
├── core_engine/
│   └── AetherDriftEngine.cpp      # Pure C++ Event loop & state execution engine
├── analytics_pipeline/
│   └── backtest_analytics.py      # Python-based performance tracking pipeline
├── data/
│   └── .gitkeep                   # Local placeholder folder for raw csv streams
├── .gitignore                     # Excludes local environments and data files
└── README.md                      # Documentation
```


## System Verification Logs

### Core Engine Output (Deterministic State Machine)
```text
=== Initializing AetherDrift Trading System Core Engine ===

Processing Market Frame -> Price: 100 | Vol: 1500
Processing Market Frame -> Price: 101.5 | Vol: 1200
Processing Market Frame -> Price: 102 | Vol: 1100
Processing Market Frame -> Price: 100.5 | Vol: 1300
Processing Market Frame -> Price: 99 | Vol: 1800
Processing Market Frame -> Price: 97.5 | Vol: 2000
Processing Market Frame -> Price: 96 | Vol: 2500
Processing Market Frame -> Price: 98 | Vol: 2200
Processing Market Frame -> Price: 100.2 | Vol: 1900
Processing Market Frame -> Price: 102.5 | Vol: 2400
[EXECUTION SUCCESS]: SELL order processed dynamically: ORD_1
Processing Market Frame -> Price: 105 | Vol: 3000
[EXECUTION SUCCESS]: BUY order processed dynamically: ORD_2
Processing Market Frame -> Price: 108.5 | Vol: 3500
[EXECUTION SUCCESS]: BUY order processed dynamically: ORD_3

Execution terminated cleanly. Total Validated Events Logged: 3
```