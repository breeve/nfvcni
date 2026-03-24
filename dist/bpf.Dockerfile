FROM nfvcni-base:latest

WORKDIR /workspace

# 复制 Makefile 和项目文件
COPY Makefile .
COPY go.mod .
COPY pkg/ ./pkg/

RUN make deps
