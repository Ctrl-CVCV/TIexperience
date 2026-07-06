# MSPM0G3519开发笔记
## 一、关于DAP烧录配置
在烧录时，配置选择pyocd，目标芯片名称改为MSPM0G3519。
在此之前，需要安装pyOCD 和 MSPM0 的 pack：
```
pip install pyocd
pyocd pack install MSPM0G3519

```
其中可能会存在pyOCD 0.44.1 处理 MSPM0GX51X_DFP 1.0.0 包的 Flash 区域定义时，两个不重叠的内存范围触发了 MemoryRange 断言失败的问题，需要做如下修改：在创建 MemoryRange 前增加重叠检测，非重叠区域直接跳过

.eide/eide.yml处做如下修改：pyOCD 配置：baseAddr 改为 0x0，targetName 改为 mspm0g3519，speed 降为 1M
然后再.eide/g3519_uart.pyocd.yaml中填写有效的配置文件