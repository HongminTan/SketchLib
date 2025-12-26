## DPDK大页管理

### 查看

```bash
cat /proc/meminfo | grep -i huge
```

### 分配

```bash
echo 1024 > /proc/sys/vm/nr_hugepages
```
| root用户执行，sudo可能无效

### 清除

#### 清除空闲的大页
```bash
echo 0 > /proc/sys/vm/nr_hugepages
```
| root用户执行，sudo可能无效

#### 将被占用的大页变为空闲

```bash
rm -f /dev/hugepages/*
```
| 有些dpdk程序异常退出后可能不会清理占用的大页，需要手动清理

## DPDK网卡管理

### 加载 VFIO 驱动模块

```bash
sudo modprobe vfio-pci
```
| root执行 `modprobe vfio-pci` 可能无效，最好加上sudo

### 关闭网卡接口

```bash
sudo ip link set eno2np1 down
```

### 绑定vfio-pci

```bash
sudo dpdk-devbind.py -b vfio-pci 0000:19:00.1
```

### 恢复

```bash
sudo dpdk-devbind.py -b bnxt_en 0000:19:00.1
```
| `sudo dpdk-devbind.py -u 0000:19:00.1` 并不能恢复网卡，因为需要 `bnxt_en` 驱动

