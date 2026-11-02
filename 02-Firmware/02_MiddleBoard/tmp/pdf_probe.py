import sys

import pdfplumber


pdf_path = sys.argv[1]
keys = [
    "接线",
    "电源",
    "供电",
    "RS485",
    "485",
    "D.R304B",
    "D.R304 B",
    "航空插头",
    "端子",
    "红",
    "黑",
    "黄",
    "绿",
    "变送器",
]

with pdfplumber.open(pdf_path) as pdf:
    print("pages", len(pdf.pages))
    for index, page in enumerate(pdf.pages, 1):
        text = page.extract_text() or ""
        hits = [key for key in keys if key in text]
        if not hits:
            continue
        print(f"---PAGE {index} HITS {','.join(hits)}")
        for line in text.splitlines():
            if any(key in line for key in keys):
                print(line[:240])
