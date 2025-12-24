## 配置Hugepages

### 查看

```bash
cat /proc/meminfo | grep -i huge
```

### 分配

```bash
echo 512 > /proc/sys/vm/nr_hugepages
```

### 清除

```bash
echo 0 > /proc/sys/vm/nr_hugepages
```