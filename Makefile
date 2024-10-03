# Makefile for Upkie spine and agent targets
#
# SPDX-License-Identifier: Apache-2.0

# Project name needs to match the one in WORKSPACE
PROJECT_NAME = contact_agent

BAZEL = $(CURDIR)/tools/bazelisk
CURDIR_NAME = $(shell basename $(CURDIR))

venv:  ## create a virtual environment
	python3 -m venv venv
	. venv/bin/activate && pip install -r requirements.txt

ENVIRONMENT = track
VALID_ENV_LIST = track stairs

.PHONY: run_agent
run_agent: venv
	@if [ -z "$(filter $(ENVIRONMENT),$(VALID_ENV_LIST))" ]; then \
		echo "ERROR: Argument $(ENVIRONMENT) is not in the list of valid environments: [$(VALID_ENV_LIST)]"; \
		exit 1; \
	fi
	$(BAZEL) build //spines:bullet_spine
	$(BAZEL) run //spines:bullet_spine -- --extra-urdf-path assets/$(ENVIRONMENT).urdf --show &
	. venv/bin/activate && python3 agent/pink_balancer/run_agent.py -c bullet

.PHONY: visualize
visualize:  ## Get the latest log file in tmp
	$(eval LATEST_LOG+=/tmp/$(shell ls /tmp/ | grep '.mpack' | tail -n 1))
	. venv/bin/activate && foxplot $(LATEST_LOG) -l /observation/contact_filter/p_contact_smooth /observation/transition_model/p_landing_p_switch /observation/transition_model/p_takeoff_p_switch
