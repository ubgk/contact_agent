Train a neural measurement network (NMN) on a simulated Upkie.

## Setup

```console
conda create -f environment.yaml -n nmn_training
conda activate nmn_training
```

## Usage

### Testing the NMN

Run your Upkie spine, then start the test agent:

```console
python test.py
```

### Training a new NMN

```
python train.py
```

### Regenerating training and validation sets

- Start a simulation spine: [`./start_simulation.sh`](https://github.com/upkie/upkie/tree/b74a4bf92daf60ce1c3c519efc6a23707bafff25?tab=readme-ov-file#getting-started)
- Run `python collect_dataset.py`
