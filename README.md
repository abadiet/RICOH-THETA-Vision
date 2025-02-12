# RICOH THETA Vision

A fast RICOH THETA USB streamer for machine vision in the French Robotics Cup.

> [!CAUTION]
> This project is abandoned as the camera as a too high internal latency.

## Dependencies

Install:
```sudo apt install -y v4l2loopback-dkms libusb-1.0-0-dev ffmpeg cmake```

v4l2loopback setup:
```sudo modprobe v4l2loopback```

## TODO

- [ ] Is BGR24 the best option?
- [ ] GPU
- [ ] SIMD
- [x] Local stream
- [ ] Benchmark

## References

- [RICOH THETA API](https://github.com/ricohapi/theta-api-specs)
- [LibUVC](https://github.com/ricohapi/libuvc-theta), a libUSB-based library to communicate with RICOH THETA
