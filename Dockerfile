FROM ubuntu:18.04
MAINTAINER  michalwidera

# update and install dependencies
RUN         apt-get update \
                && apt-get -y upgrade \
                && apt-get install -y software-properties-common \
                && apt-get install -y openssh-client \
                && apt-get install -y make \
                && apt-get install -y git \
                && apt-get install -y openssl \
                && apt-get install -y make \
                && apt-get install -y build-essential \
                && apt-get install -y cppcheck \
                && apt-get install -y cmake \
                && apt-get install -y gcc \
			    && apt-get install -y libboost-all-dev

# Set environment variables.
ENV HOME /root

# Define working directory.
WORKDIR /root

# Define default command.
CMD ["bash"]