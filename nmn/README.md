Train a neural measurement network (NMN) on a simulated Upkie.

## Setup

```console
conda create -f environment.yaml -n nmn_training
conda activate nmn_training
```

## Usage

To test the existing NMN, start your Upkie spine and run the test agent:

```console
python test.py
```

To train a new NMN, simply run `python train.py`.

Finally, to regenerate training and validation sets:

- Start a simulation spine: [`./start_simulation.sh`](https://github.com/upkie/upkie/tree/b74a4bf92daf60ce1c3c519efc6a23707bafff25?tab=readme-ov-file#getting-started)
- Run `python collect_dataset.py`
