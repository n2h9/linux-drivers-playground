## Auto load kernel module on boot
1) put module under extra folder:
```sh
sudo cp my_module.ko /lib/modules/$(uname -r)/extra/
```

2) update deps: 
```sh
sudo depmod
```

3) add file to /etc/modules-load.d/my_module.conf with content `my_module`
