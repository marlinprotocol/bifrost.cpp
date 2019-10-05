from ubuntu:18.04

COPY ./build/bifrost /root/bifrost

ENTRYPOINT ["/root/bifrost"]
