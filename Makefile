all: $(TARGET)

#The docker image name to use
DOCKER_IMAGE_NAME := microchip/xc16

#The goals to defer to the container. All others run locally
DOCKER_GOALS := all bin

#Everything below this line is boilerplate. Any configuration should be the parameters above here
#------------------------------------------------------------------------------------------------


.PHONY: docker clean





#make use_docker a prerequisite for the goals listed in DOCKER_GOALS
$(foreach goal,$(DOCKER_GOALS),$(eval $(goal): use_docker))
#is equivalent to these lines below, if DOCKER_GOALS == all bin
#all: use_docker
#bin: use_docker



#Describe the git revision if GIT_TAG not passed in from environment
#git will not exist in the container, so read it here, and pass it in.
ifndef GIT_TAG
GIT_TAG = $(shell git describe --tags --always 2>/dev/null)
endif

TOP_DIR = $(shell git rev-parse --show-toplevel)

#Set MAKECMDGOALS to default goal (usually, this is "all" ) if no goal specified
ifeq ($(MAKECMDGOALS),)
 MAKECMDGOALS = $(.DEFAULT_GOAL)
endif

#Look for the docker image
FOUND_DOCKER_IMAGE :=
#Only for the goals we defined as DOCKER_GOALS above
ifneq ($(filter $(MAKECMDGOALS),$(DOCKER_GOALS)),)
 #Look for the "docker" command. This should only succeed outside the container (on the host)
 DOCKER_CMD := $(shell which docker)
 ifneq ($(DOCKER_CMD),)
  #Check that we have a docker image with the expected name
  FOUND_DOCKER_IMAGE = $(shell docker images -q $(DOCKER_IMAGE_NAME))
 endif
endif


#If we have found the docker image to run inside of, just launch make in the container
ifneq ($(FOUND_DOCKER_IMAGE),)
 $(info Making goal "$(MAKECMDGOALS)" on docker image "$(DOCKER_IMAGE_NAME)")
 #must run as the same user as locally, so files have the same permissions
 UID := $(shell id --user):$(shell id --group)
 #we want to run make in same directory we started in locally.
 THIS_DIR := $(shell pwd)
 #work out where that is, using the relative path from the git root to pwd
 REL_DIR := $(shell realpath --relative-to=$(TOP_DIR) $(THIS_DIR) )

use_docker:
	docker run -u $(UID) -v $(TOP_DIR):/data:rw --workdir /data/$(REL_DIR) --env GIT_TAG=$(GIT_TAG) $(DOCKER_IMAGE_NAME) make $(MFLAGS) $(MAKECMDGOALS)


else
#don't use docker. define the use_docker target as empty
use_docker: ;
#and include the project orignal makefile
 include Makefile.original
endif

#define a target to build the required docker image
docker:
	docker build -t $(DOCKER_IMAGE_NAME) $(TOP_DIR)/docker/$(DOCKER_IMAGE_NAME)

