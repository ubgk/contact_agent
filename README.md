# A Data-driven Contact Estimation Method for Wheeled-Biped Robots

[![Video](figs/experiment_overview.png)](https://www.youtube.com/watch?v=QemngyjAQVU)

## Getting started

1. Create a new repository from this template.
2. Search for the string "TODO" and update files accordingly
3. Replace ``LICENSE`` with the license of your choice (the default one is Apache-2.0)
4. Start listing your dependencies in ``environment.yaml``
5. Implement your agent in the ``agent`` directory.
6. Implement your C++ spines in the ``spines`` directory.

## Usage

- Install Python packages to a conda environment: ``conda create -f environment.yaml``
- Activate conda environment: ``conda activate <env_name>``
- Run the simulation spine: ``make simulate``
- Build the pi3hat spine locally: ``make build``
- Upload the full repository (with built spines) to the robot: ``make upload``
- Run the pi3hat spine: ``make run_pi3hat_spine`` (on robot)
- Run your agent: ``python ./agent/run.py``
