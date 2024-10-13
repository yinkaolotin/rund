# rund
rund is a low-level container runtime.

## how to use

create, pause, resume container. Execute a command in an existing container
```sh
rund create <container-name> /bin/sh

rund pause <container-name>

rund resume <container-name>

rund exec <container_name> px aux
```