# 最终整体验收报告

> 说明：本文档是原桌面版 Class-OpenGL（OpenGL 3.3 Core）的验收记录，随项目保留作对照。
> OpenGL ES 3.0 版本的验证结果见 README 的「在 Ubuntu 22.04 上验证」一节。

日期：2026-10-23　环境：macOS / Apple M4 / OpenGL 4.1 Metal / MSAA 4x / Debug

## 1. 遗留修复（旧示例补最终输出 pass）

阶段 11 把 `Texture` / `Cubemap` 全部改为 sRGB 内部格式后，08/09/10 的线性场景目标
没有做输出编码，画面偏暗。本次复用 `hdr_output.frag` 补上最终输出 pass。

### 1.1 `assets/shaders/hdr_output.frag`（新增 opt-in 分支）

```glsl
// Opt-in encode for the legacy LDR examples (08/09/10): their scene targets
// hold linear values and they have no HDR pass. 11/12 leave this at 0, so
// their LDR pass-through behavior is unchanged.
uniform int u_EncodeLDR = 0;
...
    // LDR path is pass-through when the scene target already holds
    // display-ready (sRGB) values. Legacy examples that store linear values
    // opt into the linear -> sRGB encode here.
    if (u_UseHDR == 0 && u_EncodeLDR != 0)
    {
        color = pow(color, vec3(1.0 / 2.2));
    }
```

`u_EncodeLDR` 默认 0；11/12 不设置该 uniform，因此在 shader 里保持默认值，
LDR 分支仍是纯直通，行为与阶段 12 完全一致。

### 1.2 `engine/examples/08_postprocess/main.cpp`

```diff
-        Shader screenShader(postVertPath, assetDir + "/shaders/post_default.frag");
+        // Final output pass: LDR branch with the explicit linear -> sRGB
+        // encode (u_EncodeLDR), since the scene targets hold linear values.
+        Shader screenShader(postVertPath, assetDir + "/shaders/hdr_output.frag");

+        // Stage 8 acceptance: the default picture must match the direct cube
+        // render, so start with the pass-through chain (all effects off).
+        selectEffect("");
+
             screenShader.Bind();
             screenShader.SetUniform("u_ScreenTexture", 0);
+            screenShader.SetUniform("u_BloomTexture", 1);
+            screenShader.SetUniform("u_UseHDR", 0);
+            screenShader.SetUniform("u_EncodeLDR", 1);
+            screenShader.SetUniform("u_UseBloom", 0);

             GLCall(glActiveTexture(GL_TEXTURE0));
             GLCall(glBindTexture(GL_TEXTURE_2D, finalTexture));
+            // Bloom is unused here, but a sampler still needs a bound texture.
+            GLCall(glActiveTexture(GL_TEXTURE1));
+            GLCall(glBindTexture(GL_TEXTURE_2D, finalTexture));
```

> 额外一行 `selectEffect("")`：阶段 8 的验收要求「默认画面与 03_cube 直出无差异」，
> 而 `ShaderPostEffect` 的 `Enabled` 默认为 true，08 从未在启动时关闭效果（阶段 8 起即如此）。
> 该行为会让本次验收项「默认直通画面」无法成立，因此补一行启动时关闭全部效果。
> 键位与渲染路径未改动。

### 1.3 `engine/examples/09_shadow/main.cpp`

新增 include：`Buffer.h`、`Framebuffer.h`、`VertexArray.h`。

```diff
+        Shader screenShader(assetDir + "/shaders/postprocess.vert",
+                            assetDir + "/shaders/hdr_output.frag");
+
+        // Off-screen scene target so the frame can be presented through the
+        // shared output shader (single linear -> sRGB encode) instead of
+        // relying on default-framebuffer sRGB support.
+        Framebuffer sceneFramebuffer(...window.GetWidth(), window.GetHeight());
+
+        VertexBuffer screenVertexBuffer(kScreenQuadVertices, sizeof(kScreenQuadVertices));
+        const VertexAttribute screenAttributes[] = {
+            {2, GL_FLOAT, false, 4 * sizeof(float), 0},
+            {2, GL_FLOAT, false, 4 * sizeof(float), 2 * sizeof(float)},
+        };
+        VertexArray screenVertexArray;
+        screenVertexArray.Bind();
+        screenVertexArray.AddVertexBuffer(screenVertexBuffer, screenAttributes, 2);
+        screenVertexArray.Unbind();
+
+        window.SetFramebufferSizeCallback([&sceneFramebuffer](int width, int height)
+        {
+            if (width > 0 && height > 0)
+            {
+                sceneFramebuffer.Resize(...);
+            }
+        });
```

主 pass：

```diff
-            // --- Main pass ---
+            // --- Main pass: render into the off-screen scene target ---
+            sceneFramebuffer.Bind();
+            GLCall(glViewport(0, 0, window.GetWidth(), window.GetHeight()));
             Renderer::Clear(0.05f, 0.06f, 0.09f, 1.0f);
```

输出 pass（`torus.Draw(shader)` 之后）：

```cpp
            // --- Final output pass: linear -> sRGB encode to the default FBO ---
            Framebuffer::Unbind();
            GLCall(glViewport(0, 0, window.GetWidth(), window.GetHeight()));
            GLCall(glDisable(GL_DEPTH_TEST));

            screenShader.Bind();
            screenShader.SetUniform("u_ScreenTexture", 0);
            screenShader.SetUniform("u_BloomTexture", 1);
            screenShader.SetUniform("u_UseHDR", 0);
            screenShader.SetUniform("u_EncodeLDR", 1);
            screenShader.SetUniform("u_UseBloom", 0);

            const unsigned int sceneTexture = sceneFramebuffer.GetColorAttachmentID();
            GLCall(glActiveTexture(GL_TEXTURE0));
            GLCall(glBindTexture(GL_TEXTURE_2D, sceneTexture));
            GLCall(glActiveTexture(GL_TEXTURE1));
            GLCall(glBindTexture(GL_TEXTURE_2D, sceneTexture));

            screenVertexArray.Bind();
            GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
            screenVertexArray.Unbind();

            screenShader.Unbind();
            GLCall(glEnable(GL_DEPTH_TEST));
```

阴影 pass、光空间矩阵、bias/PCF 键位、相机逻辑均未改动。

### 1.4 `engine/examples/10_skybox/main.cpp`

```diff
         Shader screenShader(assetDir + "/shaders/postprocess.vert",
-                            assetDir + "/shaders/post_default.frag");
+                            assetDir + "/shaders/hdr_output.frag");

             screenShader.Bind();
             screenShader.SetUniform("u_ScreenTexture", 0);
+            screenShader.SetUniform("u_BloomTexture", 1);
+            screenShader.SetUniform("u_UseHDR", 0);
+            screenShader.SetUniform("u_EncodeLDR", 1);
+            screenShader.SetUniform("u_UseBloom", 0);

             GLCall(glActiveTexture(GL_TEXTURE0));
             GLCall(glBindTexture(GL_TEXTURE_2D, finalTexture));
+            GLCall(glActiveTexture(GL_TEXTURE1));
+            GLCall(glBindTexture(GL_TEXTURE_2D, finalTexture));
```

### 1.5 GCC 可移植性修复（Ubuntu 构建所必需）

在 Ubuntu 22.04（GCC 11.4.0）上首次构建时，GCC 拒绝成员名与类型名同名的声明：

```
PostProcess.h:60: error: declaration of 'std::shared_ptr<Shader> PostProcess::EffectEntry::Shader'
changes meaning of 'Shader' [-fpermissive]
```

修复（纯重命名）：

```diff
 struct EffectEntry
 {
     std::unique_ptr<PostEffect> Effect;
-    std::shared_ptr<Shader> Shader;
+    // Named Program, not Shader: a member sharing the type's name is
+    // rejected by GCC ("changes meaning of 'Shader'").
+    std::shared_ptr<Shader> Program;
     bool Enabled = true;
 };
```

`PostProcess.cpp` 中 `entry.Shader` 相应改为 `entry.Program`（共 4 处）。此改动不改变
逻辑、渲染顺序或行为；macOS 侧重新构建并复跑 08/10/11/12 均正常，`-Wall -Wextra`
下引擎与示例仍为零警告（3 条 `third_party/stb/stb_easy_font.h` 未使用函数警告为历史
第三方代码，非本次引入）。

### 1.6 未改动的文件

- `engine/examples/11_hdr/*`、`engine/examples/12_pbr/*`：零改动。
- 除上述 `EffectEntry` 成员重命名外，其余引擎模块、`CMakeLists.txt`、示例：零改动。

## 2. 回归记录（01–12）

构建：Debug 全量构建；另以 `-Wall -Wextra` 在独立构建目录全量编译，引擎与示例 **0 警告**。

| 示例 | 构建 | 启动 | 运行 | 键位 | 无 GL 错误 / 无 Uniform 警告 |
|---|---|---|---|---|---|
| 01_triangle | ✓ | ✓ | ✓ 颜色随时间插值 | ✓ ESC 退出 | ✓ |
| 02_texture | ✓ | ✓ | ✓ container 贴图方向/色彩正确 | ✓ ESC 退出 | ✓ |
| 03_cube | ✓ | ✓ | ✓ 深度测试、旋转正常 | ✓ W 移动、ESC | ✓ |
| 04_shader_library | ✓ | ✓ | ✓ ShaderLibrary 加载 | ✓ ESC 退出 | ✓ |
| 05_lighting | ✓ | ✓ | ✓ 光照/材质/点光，观感与阶段 11 前一致 | ✓ W 移动、ESC | ✓ |
| 06_text | ✓ | ✓ | ✓ FPS/分辨率/MSAA(4x)/后端(OpenGL) 叠加层 | ✓ T 隐藏、ESC | ✓ |
| 07_backend_switch | ✓ | ✓ | ✓ OpenGL→Null→OpenGL 切换、统计打印 | ✓ B / P、ESC | ✓ |
| 08_postprocess | ✓ | ✓ | ✓ 默认直通；Gray/Invert/Edge/Blur 均生效 | ✓ 1–6、ESC | ✓ |
| 09_shadow | ✓ | ✓ | ✓ 阴影方向正确、棋盘对比度恢复、窗口缩放正常 | ✓ `[` `]`、F6、ESC | ✓ |
| 10_skybox | ✓ | ✓ | ✓ 天空盒无接缝、反射/折射、天空亮度/层次恢复 | ✓ F6/F7/`[` `]`/`-` `=`、ESC | ✓ |
| 11_hdr | ✓ | ✓ | ✓ HDR/LDR、Reinhard、Bloom，代码零改动 | ✓ F8/F9、ESC | ✓ |
| 12_pbr | ✓ | ✓ | ✓ 球阵、4 点光+方向光、阴影、IBL、LDR/HDR、Bloom | ✓ 1–6/L/F6/F7/F8/F9、ESC | ✓ |

说明：

- 09 现在先渲染到单采样 `Framebuffer`，再经输出 pass 上屏，因此场景本身不再享受窗口
  MSAA；这与任务书「明确未实现」中的「MSAA FBO resolve（场景 FBO 单采样）」一致。
- `09_shadow` 新增了 framebuffer-size 回调，缩放窗口时场景 FBO 会随之重建（已用
  900×600 缩放实测，无 GL 错误、画面比例正确）。

### Ubuntu 22.04.5 实机复跑

环境：Ubuntu 22.04.5 LTS / GCC 11.4.0 / CMake 3.22 / Xwayland + Mesa 23.2.1
llvmpipe（LLVM 15.0.7, 256 bits）/ OpenGL 4.5 (Core Profile) / MSAA 4x / Debug。
GLFW 3.4 在 Wayland 会话下默认创建原生 Wayland 窗口；截图核对时以
`WAYLAND_DISPLAY=wayland-missing` 强制 X11/Xwayland，便于 `xwininfo` / `xwd` 定位窗口。

| 示例 | 构建 | 启动运行 | 日志 | 画面核对 |
|---|---|---|---|---|
| 01_triangle | ✓ | ✓ | 无 GL 错误 / 无 Uniform 警告 | 前序阶段已核对 |
| 02_texture | ✓ | ✓ | 同上 | 前序阶段已核对 |
| 03_cube | ✓ | ✓ | 同上 | 前序阶段已核对 |
| 04_shader_library | ✓ | ✓ | 同上 | 前序阶段已核对 |
| 05_lighting | ✓ | ✓ | 同上 | 前序阶段已核对 |
| 06_text | ✓ | ✓ | 同上 | 前序阶段已核对 |
| 07_backend_switch | ✓ | ✓ | 同上 | 前序阶段已核对 |
| 08_postprocess | ✓ | ✓ | 同上 | ✓ 默认直通立方体，贴图方向/色彩正确 |
| 09_shadow | ✓ | ✓ | 同上 | ✓ 棋盘高对比、投影方向正确 |
| 10_skybox | ✓ | ✓ | 同上 | ✓ 天空盒与环面反射，亮度/层次正常 |
| 11_hdr | ✓ | ✓ | 同上 | ✓ HDR 场景与 Bloom 正常 |
| 12_pbr | ✓ | ✓ | 同上 | ✓ 7×5 金属/粗糙度球阵、多光源、阴影正常 |

说明：

- Ubuntu 侧未安装 `xdotool`，未做按键注入；键位路径以 macOS 的逐项测试为准。
- 08–12 的 Ubuntu 截图与 macOS 同名截图逐一目视比对，画面一致。
- 构建日志确认渲染器为 `llvmpipe (LLVM 15.0.7, 256 bits)`，MSAA 4x 生效。

## 3. README 更新

`README.md` 已全量刷新：

- 功能概览新增 Framebuffer / Cubemap / PostProcess / ShadowMap / MaterialPBR /
  HDR+Bloom / PBR 着色 / 天空盒等模块；
- 示例表新增 08–12 五行，并补充运行命令；
- 示例截图新增 5 张（见下）；
- 新增「离屏渲染与后处理」「阴影贴图」「HDR 与色调映射」「PBR 与 IBL」小节；
- 自检清单全量刷新并勾选；
- 验证记录更新为 macOS 01–12 与 Ubuntu 22.04 01–12 的最终结果（含 08–12 实机截图
  核对、Wayland 下强制 X11 截图的方法，以及 GCC 可移植性修复说明）；
- 「明确未实现」按渲染技术 / 工程 / 其他三类合并整理。

## 4. 新增截图（`docs/images/`，1280×720）

| 文件 | 内容 |
|---|---|
| `08_postprocess.png` | Edge 效果（白色描边） |
| `09_shadow.png` | 方向光阴影 + 高对比棋盘 |
| `10_skybox.png` | 天空盒天空 + torus 环境反射 |
| `11_hdr.png` | HDR 高光与 Bloom 辉光 |
| `12_pbr.png` | 7×5 metallic/roughness 球阵 + 彩色点光 + 阴影 |

## 5. 最终自检

- [x] 01–12 全量 Debug 构建通过，`-Wall -Wextra` 零警告
- [x] macOS 与 Ubuntu 22.04 双平台 01–12 构建、启动、运行通过
- [x] 01–12 全部启动、运行、键位响应正常
- [x] 无 GL 错误、无 "Uniform not found" 警告
- [x] 08/09/10 补输出 pass 后观感恢复（默认直通亮度、棋盘对比度、亮蓝天空）
- [x] 11/12 代码零改动、行为不变
- [x] 整帧仅 `hdr_output.frag` 一处 gamma 编码
- [x] 新增 5 张截图与 README 引用路径一致
- [x] 项目进入维护态（无新增功能）

## 6. 备注

- Ubuntu 22.04.5 实机复跑已完成：GCC 11.4.0 构建通过，01–12 启动运行无 GL 错误、
  无 "Uniform not found"；08–12 截图与 macOS 一致。Ubuntu 未安装 xdotool，未做按键
  注入，键位结论来自 macOS。
- Ubuntu 首次构建暴露出 `EffectEntry::Shader` 的 GCC 命名冲突，已重命名为 `Program`
  （见 1.5）；此为维护态下的构建缺陷修复，无功能变化。
- Ubuntu 截图通过 `xwd` 抓取后以脚本转换（XWD 头为 big-endian，像素为 LSBFirst
  BGRX）；转换脚本仅用于本次验证，未纳入仓库。
