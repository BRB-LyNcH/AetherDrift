# AetherDrift: Deterministic Event-Driven Algorithmic Trading Core

A modular, low-latency algorithmic trading platform built to evaluate strategy patterns and enforce hard risk constraints. The software runtime architecture cleanly decouples event telemetry ingestion, decision/planning strategy compilation, and target execution logic—mirroring structural patterns found in safety-critical autonomous systems.

## Key Features
- **Deterministic Event Orchestration (C++):** Built using an Object-Oriented design that processes sequential market data stream states via strict abstract interfaces.
- **Localized Risk Guardrails:** Features a validation layer targeting account state protections, intercepting invalid order sizes or deficit balances before execution.
- **Position & PnL Management:** Adds position tracking, realized/unrealized profit tracking, and stop-loss / take-profit exit rules inside the C++ core.
- **CSV Backtest Integration (C++):** The engine now supports historical backtesting from CSV feeds using the same dataset format as `analytics_pipeline/backtest_analytics.py`.
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
"""

"""