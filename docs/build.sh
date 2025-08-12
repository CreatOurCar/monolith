#! /bin/bash

echo "$(cat package.json | jq '."single-page-markdown-website".links[1] = {"text":"Home", "url": "/"}')" > package.json
echo "$(cat package.json | jq '."single-page-markdown-website".links[2] = {"text":"GitHub", "url": "https://github.com/luftaquila/monolith"}')" > package.json

echo "$(cat package.json | jq '."single-page-markdown-website".links[0] = {"text":"English", "url": "index.html"}')" > package.json
npx --yes -- single-page-markdown-website docs_ko.md --output .
mv index.html korean.html

echo "$(cat package.json | jq '."single-page-markdown-website".links[0] = {"text":"한국어", "url": "korean.html"}')" > package.json
npx --yes -- single-page-markdown-website docs_en.md --output .

echo "$(cat package.json | jq 'del(."single-page-markdown-website".links[0])')" > package.json

for f in ./*.html; do
  [ -e "$f" ] || continue
  echo "$(
    awk '
      FNR==1{done=0}
      !done && match($0,/<style[^>]*>/){
        $0 = substr($0,1,RSTART+RLENGTH-1) "'"$(tr '\r\n' '  ' < custom.css | tr -s ' ' | sed 's/[\"\\$`]/\\&/g')"'" substr($0,RSTART+RLENGTH)
        done=1
      }
      {print}
    ' "$f"
  )" > "$f"
done
