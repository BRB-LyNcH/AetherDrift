import os
import pandas as pd
import numpy as np

def run_vectorized_backtest(csv_path):
    if not os.path.exists(csv_path):
        # Generate dummy data for verification if file doesn't exist
        print(f"[INFO]: {csv_path} not found. Generating sample data for validation...")
        dates = pd.date_range(start="2026-05-01", periods=15, freq="min")
        prices = [100.0, 101.5, 102.0, 100.5, 99.0, 97.5, 96.0, 98.0, 100.2, 102.5, 105.0, 108.5, 107.0, 109.0, 110.5]
        df_dummy = pd.DataFrame({'Timestamp': dates, 'Close': prices})
        os.makedirs(os.path.dirname(csv_path), exist_ok=True)
        df_dummy.to_csv(csv_path, index=False)

    df = pd.read_csv(csv_path)
    df['Timestamp'] = pd.to_datetime(df['Timestamp'])
    df.set_index('Timestamp', inplace=True)
    
    df['Fast_MA'] = df['Close'].rolling(window=3).mean()
    df['Slow_MA'] = df['Close'].rolling(window=5).mean()
    
    df['Signal'] = 0.0
    df['Signal'] = np.where(df['Fast_MA'] > df['Slow_MA'], 1.0, 0.0)
    
    df['Market_Returns'] = np.log(df['Close'] / df['Close'].shift(1))
    df['Strategy_Returns'] = df['Market_Returns'] * df['Signal'].shift(1)
    
    cumulative_strategy = np.exp(df['Strategy_Returns'].dropna().cumsum()) - 1
    
    print("\n--- AetherDrift Analytics Sheet ---")
    print(f"Final Strategy Yield: {cumulative_strategy.iloc[-1]*100:.2f}%" if not cumulative_strategy.empty else "Yield: 0.00%")
    return df

if __name__ == "__main__":
    run_vectorized_backtest('data/historical_market_data.csv')