FROM ubuntu:latest
MAINTAINER ckoyama<ckoyama.1996@gmail.com>

RUN apt update && \
	DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
	gcc-12-riscv64-linux-gnu cmake bash build-essential

CMD ["/bin/bash"]
