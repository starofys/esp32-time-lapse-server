# Esp32Cam Time Lapse

```shell
# note h264_vaapi need root
sudo ./demo1 -e h264_vaapi -f flv -t 1 -w 1
./demo1 -e libx264 -f mp4 -t 1
./demo1 -e h264_qsv -f flv -t 1 -w 1
```

```
usage: ./demo1 --format=string [options] ...
options:
  -e, --encoder    encoder name (string [=libx264])
  -p, --port       listen port (int [=8080])
  -f, --format     out format (string)
  -w, --webvtt     enable single webvtt subtitle file output (bool [=0])
  -r, --rate       video rate (int [=25])
  -t, --timeout    clean instance timeout minute (int [=30])
  -l, --limit      max capture size (int [=2])
  -?, --help       print this message
```