# libping
使用C语言实现ping功能，移植性较好，可以代码集成于项目中。
目前在mac os、linux、android平台（需要android交叉编译）经过测试，功能完善。

```
/**
 * 主要功能函数
 * @param domain 测试的域名或者ip
 * @return ping的结果，0代表网络通畅，-1代表网络不通
 */
int ping_host_ip(const char * domain)
```