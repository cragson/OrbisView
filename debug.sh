cd source
rm orbisview.elf
export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk; export PS5_HOST=192.168.2.63; export PS5_PORT=9021 && make test