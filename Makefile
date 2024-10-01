# Makefile for Upkie spine and agent targets
#
# SPDX-License-Identifier: Apache-2.0

# Hostname or IP address of the Raspberry Pi Uses the value from the UPKIE_NAME
# environment variable, if defined. Valid usage: ``make upload UPKIE_NAME=foo``
REMOTE = ${UPKIE_NAME}

# Project name needs to match the one in WORKSPACE
PROJECT_NAME = contact_agent

BAZEL = $(CURDIR)/tools/bazelisk
COVERAGE_DIR = $(CURDIR)/bazel-out/_coverage
CURDATE = $(shell date -Iseconds)
CURDIR_NAME = $(shell basename $(CURDIR))
RASPUNZEL = $(CURDIR)/tools/raspunzel

# Help snippet adapted from:
# http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help
help:
	@echo "Host targets:\n"
	@grep -P '^[a-zA-Z0-9_-]+:.*? ## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "    \033[36m%-24s\033[0m %s\n", $$1, $$2}'
	@echo "\nRaspberry Pi targets:\n"
	@grep -P '^[a-zA-Z0-9_-]+:.*?### .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?### "}; {printf "    \033[36m%-24s\033[0m %s\n", $$1, $$2}'
	@echo ""  # manicure
.DEFAULT_GOAL := help

.PHONY: check_upkie_name
check_upkie_name:
	@if [ -z "${UPKIE_NAME}" ]; then \
		echo "ERROR: Environment variable UPKIE_NAME is not set.\n"; \
		echo "This variable should contain the robot's hostname or IP address for SSH. "; \
		echo "You can define it inline for a one-time use:\n"; \
		echo "    make some_target UPKIE_NAME=your_robot_hostname\n"; \
		echo "Or add the following line to your shell configuration:\n"; \
		echo "    export UPKIE_NAME=your_robot_hostname\n"; \
		exit 1; \
	fi

.PHONY: clean_broken_links
clean_broken_links:
	find -L $(CURDIR) -type l ! -exec test -e {} \; -delete

.PHONY: clean
clean: clean_broken_links  ## clean all local build and intermediate files
	$(BAZEL) clean --expunge

.PHONY: build
build: clean_broken_links  ## build Raspberry Pi targets
	$(BAZEL) build --config=pi64 //agent
	$(BAZEL) build --config=pi64 //spines:mock_spine
	$(BAZEL) build --config=pi64 //spines:pi3hat_spine

.PHONY: simulate
simulate:  ## start a simulation using the Bullet spine
	$(BAZEL) run //spines:bullet_spine -- --show

.PHONY: simulate_stairs
simulate_stairs:  ## start a simulation using the Bullet spine with stairs
	$(BAZEL) run //spines:bullet_spine -- --show --extra-urdf-path assets/stairs.urdf

.PHONY: simulate_track
simulate_track:  ## start a simulation using the Bullet spine with track
	$(BAZEL) run //spines:bullet_spine -- --show --extra-urdf-path assets/track.urdf

.PHONY: upload
upload: check_upkie_name build  ## upload built targets to the Raspberry Pi
	ssh $(REMOTE) sudo date -s "$(CURDATE)"
	ssh $(REMOTE) mkdir -p $(PROJECT_NAME)
	ssh $(REMOTE) sudo find $(PROJECT_NAME) -type d -name __pycache__ -user root -exec chmod go+wx {} "\;"
	rsync -Lrtu --delete-after --delete-excluded --exclude bazel-out/ --exclude bazel-testlogs/ --exclude bazel-$(CURDIR_NAME) --exclude bazel-$(PROJECT_NAME)/ --exclude cache/ --exclude logs/ --exclude training/ --progress $(CURDIR)/ $(REMOTE):$(PROJECT_NAME)/

run_mock_spine:  ### run the mock spine on the Raspberry Pi
	$(RASPUNZEL) run -s //spines:mock_spine

# NB: run_pi3hat_spine is used in build instructions
run_pi3hat_spine:  ### run the pi3hat spine on the Raspberry Pi
	$(RASPUNZEL) run -s //spines:pi3hat_spine

run_agent:  ### run the agent on the Raspberry Pi
	$(RASPUNZEL) run -s //agent:agent
