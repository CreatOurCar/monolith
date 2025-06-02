# Monolith ESP32-S3 Rust Firmware

## Prerequisites

1. Install Rust

    ```sh
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
    ```

1. Install esp-rs Rust toolchain

    ```sh
    cargo install espup
    espup install --targets esp32s3
    ```

## Build / run

```sh
make build
make run
```

### Wokwi

1. Sign up to [Wokwi simulator](https://wokwi.com/)
1. Create new [Wokwi CI Token](https://wokwi.com/dashboard/ci) and export it as `WOKWI_CLI_TOKEN`

    ```sh
    echo 'export WOKWI_CLI_TOKEN=<YOUR_WOKWI_CI_TOKEN>' >> /path/to/your/shell/rc  # ex) $HOME/.bashrc
    ```

1. Install `wokwi-cli` and restart shell

    ```sh
    curl -L https://wokwi.com/ci/install.sh | sh
    ```

1. Run simulation

    ```sh
    make sim
    ```
