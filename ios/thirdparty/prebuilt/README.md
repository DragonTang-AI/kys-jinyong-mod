# 手写 / 预编译静态库落点

将你的 **头文件** 放入本目录下的 **`include/`**：

```text
thirdparty/prebuilt/include/SDL3/SDL.h
thirdparty/prebuilt/include/lua.hpp
...
```

将 **静态库 `.a`** 放入（布局与 `ios/README-Xcode静态库.md` 一致）：

```text
thirdparty/prebuilt/iphoneos/lib/libSDL3.a
thirdparty/prebuilt/iphoneos/lib/liblua.a
...
thirdparty/prebuilt/iphonesimulator/lib/...
```

`xcconfig/Base.xcconfig` 已通过 `PLATFORM_NAME` 切换上述 `lib/` 路径。
