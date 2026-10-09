#!/usr/bin/env bash
# 產生 GitHub Pages 網站到 $1(預設 _site):合併後的韌體、ESP Web Tools 的 manifest、
# 從兩份 README 轉出的 index.html / zh-TW.html(只取到「建置」章節之前,前面插入燒錄按鈕)
# 需求:先跑過 pio run;PATH 上有 pio、esptool、python3,gh 已登入(CI 用 GH_TOKEN)
set -euo pipefail
cd "$(dirname "$0")/../.."
out=${1:-_site}
mkdir -p "$out"
cp -r previews "$out/"

# bootloader、分區表、boot_app0、app 合併成一個從 0x0 開始寫的檔,位址取自 PlatformIO
images=$(pio project metadata -e m5stack-core2 --json-output | python3 -c '
import json, sys
d = json.load(sys.stdin)["m5stack-core2"]
e = d["extra"]
print(*[i["offset"] + " " + i["path"] for i in e["flash_images"]], e["application_offset"], d["prog_path"][:-4] + ".bin")')
esptool --chip esp32 merge-bin -o "$out/firmware.bin" $images

cat > "$out/manifest.json" <<EOF
{
  "name": "m5stack-fidgets",
  "version": "$(git rev-parse --short HEAD)",
  "new_install_prompt_erase": true,
  "builds": [{ "chipFamily": "ESP32", "parts": [{ "path": "firmware.bin", "offset": 0 }] }]
}
EOF

# $1 README、$2 輸出檔、$3 lang、$4 燒錄區塊 HTML
render() {
  sed '/^## \(Build\|建置\)/,$d' "$1" | gh api markdown -f mode=gfm -f context=8loser/m5stack-fidgets -F text=@- \
    | sed 's/href="README\.zh-TW\.md"/href="zh-TW.html"/; s/href="README\.md"/href="index.html"/' \
    | sed -E 's#<a target="_blank"[^>]*>(<img[^>]*>)</a>#\1#g; s# style="max-width: 100%;"##g' > "$out/body.tmp"  # GitHub 會把圖包成開新分頁的連結,並加上蓋過 CSS 的 inline 樣式
  INSTALL=$4 LANG_=$3 python3 - "$out/body.tmp" "$2" <<'PY'
import os, sys
body = open(sys.argv[1]).read()
i = body.find("<h2")  # 燒錄區塊放在 banner 之後、第一個章節之前
body = body[:i] + os.environ["INSTALL"] + body[i:]
page = open("tools/web/template.html").read()
open(sys.argv[2], "w").write(page.replace("{{LANG}}", os.environ["LANG_"]).replace("{{BODY}}", body))
PY
  rm "$out/body.tmp"
}

render README.md "$out/index.html" en '<div class="install">
<esp-web-install-button manifest="manifest.json">
<span slot="unsupported">Flashing from the browser needs desktop Chrome, Edge or Opera.</span>
<span slot="not-allowed">Flashing from the browser needs an HTTPS page.</span>
</esp-web-install-button>
<p>Connect the Core2 over USB, then click Connect and pick its serial port. This replaces the firmware currently on the device.</p>
</div>'

render README.zh-TW.md "$out/zh-TW.html" zh-Hant '<div class="install">
<esp-web-install-button manifest="manifest.json">
<span slot="unsupported">網頁燒錄需要桌面版 Chrome、Edge 或 Opera。</span>
<span slot="not-allowed">網頁燒錄需要 HTTPS 頁面。</span>
</esp-web-install-button>
<p>用 USB 線接上 Core2,按 Connect 後選它的序列埠。這會覆蓋裝置上原本的韌體。</p>
</div>'
