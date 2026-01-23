#!/bin/bash
# 1. 清理旧缓存
npx hexo clean
# 2. 备份源码到 GitHub
git add .
git commit -m "Update blog: $(date +%Y-%m-%d)"
git push origin hexo
# 3. 发布网页
npx hexo g -d
echo "发布成功！"
