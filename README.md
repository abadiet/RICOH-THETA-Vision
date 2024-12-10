# RICOH THETA Vision

Repo dedicated to build a fast RICOH THETA USB streamer in order to be used as a vision camera during the French Robotics Cup.

## Benchmark

*Comming soon*

## Dependencies

Install:
```sudo apt install -y v4l2loopback-dkms libusb-1.0-0-dev ffmpeg cmake```

v4l2loopback setup:
```sudo modprobe v4l2loopback```

## TODO

- [ ] Is BGR24 the best option?
- [ ] GPU
- [ ] SIMD
- [ ] Local stream
- [ ] Benchmark

## References

- [RICOH THETA API](https://github.com/ricohapi/theta-api-specs)
- [LibUVC](https://github.com/ricohapi/libuvc-theta), a libUSB-based library to communicate with RICOH THETA
