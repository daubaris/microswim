FROM alpine:3.21

WORKDIR /src

# Install all dependencies required to build microswim
RUN apk upgrade && apk update && \
    apk add --no-cache util-linux-dev linux-headers musl-dev make cmake gcc g++ \
    hiredis-dev libuuid libcbor-dev sqlite-dev benchmark-dev bash

# Use bash shell for scripting convenience
SHELL ["/bin/bash", "-lc"]

# Default command: stay alive, ready for exec commands
CMD ["sleep", "infinity"]
