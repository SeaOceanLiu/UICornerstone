## 1. 概述

NumericUpDown（数值微调控件）是一种允许用户通过**直接键入**或**点击步进按钮**在指定范围内调节数值的 UI 控件，类似于 HTML 中的 `<input type="number">` 或 WinForms 的 `NumericUpDown`。支持范围限制、可配置步长、小数精度、占位符、只读模式、长按连续步进等功能。

### 1.1 视觉结构

> 以下 SVG 图为内联嵌入式，可直接在文档中渲染。如需编辑可将 SVG 代码导出为独立文件后在 [draw.io](https://app.diagrams.net) 编辑（File → Import → SVG → 选择文件）。

#### 1.1.1 默认状态

<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 480 200" width="460" height="190">
  <defs>
    <linearGradient id="bgGrad1" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#3A3A3A"/>
      <stop offset="100%" stop-color="#2D2D2D"/>
    </linearGradient>
    <linearGradient id="btnN1" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#5A5A5A"/>
      <stop offset="100%" stop-color="#454545"/>
    </linearGradient>
  </defs>
  <text x="8" y="18" font-family="Arial,sans-serif" font-size="12" font-weight="bold" fill="#E0E0E0">默认状态 (1× scale, w=120, h=24)</text>
  <g transform="translate(16, 36)">
    <rect x="0" y="0" width="160" height="32" rx="4" fill="url(#bgGrad1)" stroke="#666" stroke-width="1"/>
    <text x="14" y="21" font-family="Consolas,monospace" font-size="13" fill="#E0E0E0">100</text>
    <line x1="128" y1="6" x2="128" y2="26" stroke="#444" stroke-width="0.8"/>
    <rect x="128" y="1" width="31" height="15" fill="url(#btnN1)"/><polygon points="143,5 138,12 148,12" fill="#D0D0D0"/>
    <line x1="128" y1="16" x2="159" y2="16" stroke="#222" stroke-width="0.8"/>
    <rect x="128" y="16" width="31" height="15" fill="url(#btnN1)"/><polygon points="143,27 138,20 148,20" fill="#D0D0D0"/>
    <line x1="65" y1="34" x2="65" y2="44" stroke="#888" stroke-width="0.5" stroke-dasharray="2,2"/>
    <text x="36" y="56" font-family="Arial,sans-serif" font-size="9" fill="#888">EditBox 区 (w-16)</text>
    <line x1="143" y1="34" x2="143" y2="44" stroke="#888" stroke-width="0.5" stroke-dasharray="2,2"/>
    <text x="127" y="56" font-family="Arial,sans-serif" font-size="9" fill="#888">Button 区 (16)</text>
  </g>
  <g transform="translate(16, 112)">
    <text x="0" y="-4" font-family="Arial,sans-serif" font-size="9" fill="#888">负数 + 小数精度示例 (range -100~100, step=5)</text>
    <rect x="0" y="0" width="160" height="32" rx="4" fill="url(#bgGrad1)" stroke="#666" stroke-width="1"/>
    <text x="14" y="21" font-family="Consolas,monospace" font-size="13" fill="#E0E0E0">-50</text>
    <line x1="128" y1="6" x2="128" y2="26" stroke="#444" stroke-width="0.8"/>
    <rect x="128" y="1" width="31" height="15" fill="url(#btnN1)"/><polygon points="143,5 138,12 148,12" fill="#D0D0D0"/>
    <line x1="128" y1="16" x2="159" y2="16" stroke="#222" stroke-width="0.8"/>
    <rect x="128" y="16" width="31" height="15" fill="url(#btnN1)"/><polygon points="143,27 138,20 148,20" fill="#D0D0D0"/>
  </g>
</svg>

*左侧 EditBox 显示数值 "100"，右侧 16px 宽的按钮区分为上下两部分，分别绘制 ▲ 和 ▼ 三角形。负数示例展示 `-50` 的渲染。*

#### 1.1.2 Hover / Press 状态

<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 540 260" width="520" height="250">
  <defs>
    <linearGradient id="bgGrad2" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#3A3A3A"/><stop offset="100%" stop-color="#2D2D2D"/></linearGradient>
    <linearGradient id="btnN2" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#5A5A5A"/><stop offset="100%" stop-color="#454545"/></linearGradient>
    <linearGradient id="btnH2" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#787878"/><stop offset="100%" stop-color="#606060"/></linearGradient>
    <linearGradient id="btnP2" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#909090"/><stop offset="100%" stop-color="#707070"/></linearGradient>
    <marker id="arrow2" markerWidth="6" markerHeight="6" refX="5" refY="3" orient="auto">
      <polygon points="0 0, 6 3, 0 6" fill="#888"/>
    </marker>
  </defs>
  <text x="8" y="18" font-family="Arial,sans-serif" font-size="12" font-weight="bold" fill="#E0E0E0">三态对比 (Normal / Hover / Press)</text>
  <!-- Normal -->
  <g transform="translate(12, 36)">
    <text x="0" y="-4" font-family="Arial,sans-serif" font-size="9" fill="#888">Normal</text>
    <rect x="0" y="0" width="160" height="32" rx="4" fill="url(#bgGrad2)" stroke="#666" stroke-width="1"/>
    <text x="14" y="21" font-family="Consolas,monospace" font-size="13" fill="#E0E0E0">100</text>
    <line x1="128" y1="6" x2="128" y2="26" stroke="#444" stroke-width="0.8"/>
    <rect x="128" y="1" width="31" height="15" fill="url(#btnN2)"/><polygon points="143,5 138,12 148,12" fill="#D0D0D0"/>
    <line x1="128" y1="16" x2="159" y2="16" stroke="#222" stroke-width="0.8"/>
    <rect x="128" y="16" width="31" height="15" fill="url(#btnN2)"/><polygon points="143,27 138,20 148,20" fill="#D0D0D0"/>
  </g>
  <!-- Hover -->
  <g transform="translate(184, 36)">
    <text x="0" y="-4" font-family="Arial,sans-serif" font-size="9" fill="#888">Hover (Up 按钮)</text>
    <rect x="0" y="0" width="160" height="32" rx="4" fill="url(#bgGrad2)" stroke="#666" stroke-width="1"/>
    <text x="14" y="21" font-family="Consolas,monospace" font-size="13" fill="#E0E0E0">100</text>
    <line x1="128" y1="6" x2="128" y2="26" stroke="#444" stroke-width="0.8"/>
    <rect x="128" y="1" width="31" height="15" fill="url(#btnH2)"/><polygon points="143,5 138,12 148,12" fill="#FFF"/>
    <line x1="128" y1="16" x2="159" y2="16" stroke="#222" stroke-width="0.8"/>
    <rect x="128" y="16" width="31" height="15" fill="url(#btnN2)"/><polygon points="143,27 138,20 148,20" fill="#D0D0D0"/>
  </g>
  <!-- Press -->
  <g transform="translate(356, 36)">
    <text x="0" y="-4" font-family="Arial,sans-serif" font-size="9" fill="#888">Press (Up 按钮)</text>
    <rect x="0" y="0" width="160" height="32" rx="4" fill="url(#bgGrad2)" stroke="#666" stroke-width="1"/>
    <text x="14" y="21" font-family="Consolas,monospace" font-size="13" fill="#E0E0E0">101</text>
    <line x1="128" y1="6" x2="128" y2="26" stroke="#444" stroke-width="0.8"/>
    <rect x="128" y="1" width="31" height="15" fill="url(#btnP2)"/><polygon points="143,5 138,12 148,12" fill="#FF0"/>
    <line x1="128" y1="16" x2="159" y2="16" stroke="#222" stroke-width="0.8"/>
    <rect x="128" y="16" width="31" height="15" fill="url(#btnN2)"/><polygon points="143,27 138,20 148,20" fill="#D0D0D0"/>
  </g>
  <!-- 状态机 -->
  <g transform="translate(12, 130)">
    <text x="0" y="0" font-family="Arial,sans-serif" font-size="9" fill="#AAA">状态迁移：</text>
    <circle cx="40" cy="24" r="12" fill="url(#btnN2)" stroke="#888" stroke-width="1"/>
    <text x="40" y="52" font-family="Arial,sans-serif" font-size="8" fill="#888" text-anchor="middle">Normal</text>
    <circle cx="120" cy="24" r="12" fill="url(#btnH2)" stroke="#888" stroke-width="1"/>
    <text x="120" y="52" font-family="Arial,sans-serif" font-size="8" fill="#888" text-anchor="middle">Hover</text>
    <circle cx="200" cy="24" r="12" fill="url(#btnP2)" stroke="#888" stroke-width="1"/>
    <text x="200" y="52" font-family="Arial,sans-serif" font-size="8" fill="#888" text-anchor="middle">Press</text>
    <line x1="52" y1="24" x2="108" y2="24" stroke="#888" stroke-width="0.8" marker-end="url(#arrow2)"/>
    <text x="80" y="18" font-family="Arial,sans-serif" font-size="7" fill="#888" text-anchor="middle">mouseEnter</text>
    <line x1="132" y1="24" x2="188" y2="24" stroke="#888" stroke-width="0.8" marker-end="url(#arrow2)"/>
    <text x="160" y="18" font-family="Arial,sans-serif" font-size="7" fill="#888" text-anchor="middle">MouseDown</text>
    <line x1="188" y1="34" x2="132" y2="34" stroke="#888" stroke-width="0.8" marker-end="url(#arrow2)"/>
    <text x="160" y="46" font-family="Arial,sans-serif" font-size="7" fill="#888" text-anchor="middle">MouseUp</text>
    <line x1="108" y1="34" x2="52" y2="34" stroke="#888" stroke-width="0.8" marker-end="url(#arrow2)"/>
    <text x="80" y="46" font-family="Arial,sans-serif" font-size="7" fill="#888" text-anchor="middle">mouseLeave</text>
  </g>
</svg>

*三态对比：Normal (浅灰)、Hover (中灰，箭头变白)、Press (深灰，箭头变黄)。下方状态机图展示 Normal → Hover → Press → Normal 的迁移条件。*

#### 1.1.3 焦点环（3 层）

<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 480 260" width="460" height="250">
  <defs>
    <linearGradient id="bgGrad3" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#3A3A3A"/><stop offset="100%" stop-color="#2D2D2D"/></linearGradient>
    <linearGradient id="btnN3" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#5A5A5A"/><stop offset="100%" stop-color="#454545"/></linearGradient>
  </defs>
  <text x="8" y="18" font-family="Arial,sans-serif" font-size="12" font-weight="bold" fill="#E0E0E0">3 层焦点环 (2026-07-07)</text>
  <!-- 1x -->
  <g transform="translate(12, 40)">
    <text x="0" y="-4" font-family="Arial,sans-serif" font-size="9" fill="#888">1× scale</text>
    <rect x="-2" y="-2" width="164" height="36" rx="5" fill="none" stroke="rgba(0,0,0,0.6)" stroke-width="1"/>
    <rect x="-1" y="-1" width="162" height="34" rx="5" fill="none" stroke="rgba(255,255,255,0.6)" stroke-width="1"/>
    <rect x="0" y="0" width="160" height="32" rx="4" fill="url(#bgGrad3)" stroke="#0078FF" stroke-width="1.5"/>
    <text x="14" y="21" font-family="Consolas,monospace" font-size="13" fill="#E0E0E0">100</text>
    <line x1="128" y1="6" x2="128" y2="26" stroke="#444" stroke-width="0.8"/>
    <rect x="128" y="1" width="31" height="15" fill="url(#btnN3)"/><polygon points="143,5 138,12 148,12" fill="#D0D0D0"/>
    <line x1="128" y1="16" x2="159" y2="16" stroke="#222" stroke-width="0.8"/>
    <rect x="128" y="16" width="31" height="15" fill="url(#btnN3)"/><polygon points="143,27 138,20 148,20" fill="#D0D0D0"/>
  </g>
  <!-- 2x -->
  <g transform="translate(200, 40)">
    <text x="0" y="-4" font-family="Arial,sans-serif" font-size="9" fill="#888">2× scale</text>
    <rect x="-2" y="-2" width="244" height="52" rx="6" fill="none" stroke="rgba(0,0,0,0.6)" stroke-width="1"/>
    <rect x="-1" y="-1" width="242" height="50" rx="6" fill="none" stroke="rgba(255,255,255,0.6)" stroke-width="1"/>
    <rect x="0" y="0" width="240" height="48" rx="5" fill="url(#bgGrad3)" stroke="#0078FF" stroke-width="2"/>
    <text x="20" y="31" font-family="Consolas,monospace" font-size="20" fill="#E0E0E0">100</text>
    <line x1="208" y1="8" x2="208" y2="40" stroke="#444" stroke-width="1.2"/>
    <rect x="208" y="1" width="32" height="23" fill="url(#btnN3)"/><polygon points="224,7 216,19 232,19" fill="#D0D0D0"/>
    <line x1="208" y1="24" x2="240" y2="24" stroke="#222" stroke-width="1.2"/>
    <rect x="208" y="24" width="32" height="23" fill="url(#btnN3)"/><polygon points="224,42 216,30 232,30" fill="#D0D0D0"/>
  </g>
  <g transform="translate(12, 130)">
    <text x="0" y="0" font-family="Arial,sans-serif" font-size="10" font-weight="bold" fill="#E0E0E0">3 层结构</text>
    <rect x="0" y="12" width="16" height="12" fill="none" stroke="rgba(0,0,0,0.6)" stroke-width="1"/><text x="22" y="22" font-family="Arial,sans-serif" font-size="9" fill="#CCC">Layer 1 — 黑, inset=0, alpha=150</text>
    <rect x="0" y="28" width="16" height="12" fill="none" stroke="rgba(255,255,255,0.6)" stroke-width="1"/><text x="22" y="38" font-family="Arial,sans-serif" font-size="9" fill="#CCC">Layer 2 — 白, inset=1, alpha=150</text>
    <rect x="0" y="44" width="16" height="12" fill="url(#bgGrad3)" stroke="#0078FF" stroke-width="1.5"/><text x="22" y="54" font-family="Arial,sans-serif" font-size="9" fill="#CCC">Layer 3 — 用户色, inset=2 (#0078FF)</text>
  </g>
</svg>

*1× 和 2× 缩放下都显示 3 层焦点环：黑 (inset=0, alpha=150) + 白 (inset=1, alpha=150) + 用户色 (inset=2, 默认 #0078FF)，保证任何背景色下至少一条线可见。*

#### 1.1.4 缩放感知

<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 520 260" width="500" height="250">
  <defs>
    <linearGradient id="bgGrad4" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#3A3A3A"/><stop offset="100%" stop-color="#2D2D2D"/></linearGradient>
    <linearGradient id="btnN4" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#5A5A5A"/><stop offset="100%" stop-color="#454545"/></linearGradient>
  </defs>
  <text x="8" y="18" font-family="Arial,sans-serif" font-size="12" font-weight="bold" fill="#E0E0E0">缩放感知 (1× vs 2×)</text>
  <!-- 1x -->
  <g transform="translate(12, 38)">
    <text x="0" y="-4" font-family="Arial,sans-serif" font-size="9" fill="#888">1× — rect=120×24, buttonWidth=16, fontSize=12</text>
    <rect x="0" y="0" width="120" height="24" rx="3" fill="url(#bgGrad4)" stroke="#666" stroke-width="1"/>
    <text x="10" y="16" font-family="Consolas,monospace" font-size="12" fill="#E0E0E0">100</text>
    <line x1="104" y1="4" x2="104" y2="20" stroke="#444" stroke-width="0.8"/>
    <rect x="104" y="0" width="16" height="12" fill="url(#btnN4)"/><polygon points="112,3 109,9 115,9" fill="#D0D0D0"/>
    <line x1="104" y1="12" x2="120" y2="12" stroke="#222" stroke-width="0.8"/>
    <rect x="104" y="12" width="16" height="12" fill="url(#btnN4)"/><polygon points="112,21 109,15 115,15" fill="#D0D0D0"/>
  </g>
  <!-- 2x -->
  <g transform="translate(12, 90)">
    <text x="0" y="-4" font-family="Arial,sans-serif" font-size="9" fill="#888">2× — rect=120×24, 实际渲染 240×48, fontSize=24</text>
    <rect x="0" y="0" width="240" height="48" rx="6" fill="url(#bgGrad4)" stroke="#666" stroke-width="1"/>
    <text x="20" y="32" font-family="Consolas,monospace" font-size="24" fill="#E0E0E0">100</text>
    <line x1="208" y1="8" x2="208" y2="40" stroke="#444" stroke-width="1.5"/>
    <rect x="208" y="0" width="32" height="24" fill="url(#btnN4)"/><polygon points="224,6 218,18 230,18" fill="#D0D0D0"/>
    <line x1="208" y1="24" x2="240" y2="24" stroke="#222" stroke-width="1.5"/>
    <rect x="208" y="24" width="32" height="24" fill="url(#btnN4)"/><polygon points="224,42 218,30 230,30" fill="#D0D0D0"/>
  </g>
  <g transform="translate(12, 160)">
    <text x="0" y="0" font-family="Arial,sans-serif" font-size="9" fill="#AAA">自动缩放：m_rect | m_buttonWidth | EditBox 字体 | 三角形顶点 | hitRect/drawRect</text>
    <text x="0" y="18" font-family="Consolas,monospace" font-size="8" fill="#F88">⚠ 禁止双重缩放：ebRect = {0, 0, w - bw, h} （未缩放，EditBox 内部再缩放）</text>
  </g>
</svg>

*1× (120×24) vs 2× (240×48) 对比。所有几何量自动按 `getScaleXX()/getScaleYY()` 缩放，包括 button 区宽度、EditBox 字体、三角形顶点。*

#### 1.1.5 长按连续步进时序

<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 620 300" width="600" height="290">
  <defs>
    <marker id="arrowR" markerWidth="10" markerHeight="6" refX="9" refY="3" orient="auto"><polygon points="0 0,10 3,0 6" fill="#888"/></marker>
    <filter id="shadow5"><feDropShadow dx="1" dy="1" stdDeviation="1" flood-opacity="0.3"/></filter>
  </defs>
  <text x="8" y="16" font-family="Arial,sans-serif" font-size="11" font-weight="bold" fill="#E0E0E0">长按重复时序</text>
  <text x="8" y="30" font-family="Arial,sans-serif" font-size="9" fill="#888">kRepeatDelaySec = 500ms, kRepeatIntervalSec = 100ms</text>
  <line x1="20" y1="65" x2="600" y2="65" stroke="#888" stroke-width="1.5" marker-end="url(#arrowR)"/>
  <text x="608" y="69" font-family="Arial,sans-serif" font-size="10" fill="#888">t</text>
  <line x1="20" y1="60" x2="20" y2="70" stroke="#888" stroke-width="1"/><text x="16" y="57" font-family="Arial,sans-serif" font-size="9" fill="#AAA">0ms</text>
  <line x1="120" y1="60" x2="120" y2="70" stroke="#888" stroke-width="1"/><text x="112" y="57" font-family="Arial,sans-serif" font-size="9" fill="#AAA">500ms</text>
  <line x1="220" y1="60" x2="220" y2="70" stroke="#888" stroke-width="1"/><text x="212" y="57" font-family="Arial,sans-serif" font-size="9" fill="#AAA">600ms</text>
  <line x1="320" y1="60" x2="320" y2="70" stroke="#888" stroke-width="1"/><text x="312" y="57" font-family="Arial,sans-serif" font-size="9" fill="#AAA">700ms</text>
  <line x1="420" y1="60" x2="420" y2="70" stroke="#888" stroke-width="1"/><text x="412" y="57" font-family="Arial,sans-serif" font-size="9" fill="#AAA">800ms</text>
  <line x1="520" y1="60" x2="520" y2="70" stroke="#888" stroke-width="1"/><text x="512" y="57" font-family="Arial,sans-serif" font-size="9" fill="#AAA">900ms</text>
  <line x1="20" y1="72" x2="20" y2="110" stroke="#FF6B6B" stroke-width="2"/><polygon points="15,110 25,110 20,120" fill="#FF6B6B"/>
  <text x="6" y="135" font-family="Arial,sans-serif" font-size="9" fill="#FF6B6B">MouseDown</text>
  <rect x="30" y="90" width="80" height="18" rx="3" fill="none" stroke="#FF6B6B" stroke-width="0.8" stroke-dasharray="4,3"/>
  <text x="38" y="102" font-family="Arial,sans-serif" font-size="8" fill="#FF6B6B">静止 (500ms 延迟)...</text>
  <rect x="128" y="90" width="380" height="18" rx="3" fill="rgba(100,200,255,0.08)" stroke="#64C8FF" stroke-width="0.5" stroke-dasharray="3,2"/>
  <text x="250" y="102" font-family="Arial,sans-serif" font-size="8" fill="#64C8FF">重复区间：每 100ms 步进一次</text>
  <circle cx="120" cy="180" r="7" fill="#64C8FF" stroke="#2A8ABF" stroke-width="1"/>
  <line x1="120" y1="72" x2="120" y2="173" stroke="#64C8FF" stroke-width="0.8" stroke-dasharray="4,2"/>
  <circle cx="220" cy="180" r="7" fill="#64C8FF" stroke="#2A8ABF" stroke-width="1"/>
  <line x1="220" y1="72" x2="220" y2="173" stroke="#64C8FF" stroke-width="0.8" stroke-dasharray="4,2"/>
  <circle cx="320" cy="180" r="7" fill="#64C8FF" stroke="#2A8ABF" stroke-width="1"/>
  <line x1="320" y1="72" x2="320" y2="173" stroke="#64C8FF" stroke-width="0.8" stroke-dasharray="4,2"/>
  <circle cx="420" cy="180" r="7" fill="#64C8FF" stroke="#2A8ABF" stroke-width="1"/>
  <line x1="420" y1="72" x2="420" y2="173" stroke="#64C8FF" stroke-width="0.8" stroke-dasharray="4,2"/>
  <circle cx="520" cy="180" r="7" fill="#64C8FF" stroke="#2A8ABF" stroke-width="1"/>
  <line x1="520" y1="72" x2="520" y2="173" stroke="#64C8FF" stroke-width="0.8" stroke-dasharray="4,2"/>
  <text x="105" y="200" font-family="Consolas,monospace" font-size="9" fill="#64C8FF" text-anchor="middle">Value 100</text>
  <text x="105" y="212" font-family="Consolas,monospace" font-size="9" fill="#64C8FF" text-anchor="middle">→ 101</text>
  <text x="205" y="212" font-family="Consolas,monospace" font-size="9" fill="#64C8FF" text-anchor="middle">103</text>
  <text x="305" y="212" font-family="Consolas,monospace" font-size="9" fill="#64C8FF" text-anchor="middle">105</text>
  <text x="405" y="212" font-family="Consolas,monospace" font-size="9" fill="#64C8FF" text-anchor="middle">107</text>
  <text x="505" y="212" font-family="Consolas,monospace" font-size="9" fill="#64C8FF" text-anchor="middle">109</text>
</svg>

*时间轴：MouseDown → 500ms 静止延迟 → 首次 stepValue() 触发 → 之后每 100ms 重复步进一次。数值变化：100 → 101 → 103 → 105 → 107 → ...*

#### 1.1.6 错误状态（值越界后回弹）

用户输入 "999"（超过 max=100）后，EditBox 文本自动替换为 max 值（100）：外观与 1.1.1 默认状态相同，EditBox 显示 `100`（自动截断），按钮区不变。

#### 1.1.7 只读模式

只读模式下 EditBox 文本编辑被禁用（`m_readOnly=true` 时 TextInput 事件被丢弃），但步进按钮仍可点击。外观与 1.1.2 正常状态相同，区别仅在交互：键盘输入无效，点击 ▲/▼ 仍可步进。

## 2. 功能规格

### 2.1 核心功能

| 编号 | 功能                     | 优先级 | 说明                                               |
|------|--------------------------|--------|----------------------------------------------------|
| F1   | 数值显示与编辑           | P0     | EditBox 显示当前数值，支持直接键盘输入              |
| F2   | 步进增/减按钮            | P0     | 右侧 ▲/▼ 按钮，单击 +step / -step                   |
| F3   | 长按连续步进             | P0     | 首次延迟 500ms，之后每 100ms 步进一次               |
| F4   | 范围限制                 | P0     | min/max 钳位，越界输入自动截断                       |
| F5   | 步长配置                 | P0     | step 可配置（默认 1，支持 0.1、10、100 等）         |
| F6   | 键盘步进                 | P0     | ↑/PageUp 按 pageStep 步进（默认 10）；↓/PageDown 同理 |
| F7   | 范围跳转                 | P0     | Home = min；End = max                                |
| F8   | 小数精度                 | P1     | decimals 控制小数位数（0-6，默认 0）                |
| F9   | 初始值和占位符           | P1     | 可设默认值；空值时显示 placeholder 文本             |
| F10  | 只读模式                 | P2     | 禁止 EditBox 编辑，步进按钮仍可用                   |
| F11  | 缩放支持                 | P0     | 全控件适配 scaleX/scaleY                            |
| F12  | 焦点管理                 | P0     | Tab 进入/离开、3 层焦点环、FocusLost 提交值         |
| F13  | 事件回调                 | P0     | onValueChanged 在值变化时触发                       |
| F14  | JSON 布局支持            | P0     | `type: "NumericUpDown"` 解析                        |
| F15  | C ABI 导出               | P0     | 12 个 C ABI 函数（工厂/值/范围/步长/pageStep/精度/回调）      |

### 2.2 交互行为

| 触发                 | 行为                                                    |
|----------------------|---------------------------------------------------------|
| 单击 ▲               | `value += step`，触发 onValueChanged                     |
| 单击 ▼               | `value -= step`，触发 onValueChanged                     |
| 长按 ▲               | 延迟 500ms 后每 100ms 触发 `value += step`               |
| 长按 ▼               | 延迟 500ms 后每 100ms 触发 `value -= step`               |
| ↑ / +                | `value += step`                                          |
| ↓ / -                | `value -= step`                                          |
| PageUp               | `value += pageStep`（默认 10.0）                        |
| PageDown             | `value -= pageStep`                                      |
| Home                 | `value = min`                                            |
| End                  | `value = max`                                            |
| 数字/./- 键          | EditBox 直接接收（需校验）                               |
| Enter                | 提交 EditBox 文本 → 解析 → 钳位 → 触发回调              |
| FocusLost            | 自动提交 EditBox 文本（同 Enter）                       |
| Tab                  | 焦点离开 NumericUpDown 到下一个可聚焦控件               |

### 2.3 输入校验规则

| 输入             | 处理                                                            |
|------------------|-----------------------------------------------------------------|
| 空字符串         | `value = min`（F9 占位符恢复）                                  |
| 合法数字         | 解析 → 钳位到 [min, max] → 按 step 量化 → 触发回调              |
| 非数字字符       | 在 EditBox 输入时过滤（数字、`-`、`.`）                          |
| NaN              | 保留旧值，不更新                                                |
| 解析失败         | EditBox 文本回滚为旧值的格式化字符串                             |

### 2.4 边界条件

| 边界                 | 行为                                                       |
|----------------------|------------------------------------------------------------|
| `min == max`         | 所有输入/步进均设为该值，不触发回调                          |
| `min > max`          | 构造时 swap，或断言失败（采用断言 + 警告）                  |
| `step == 0`          | 步进按钮无效果（assert）；键盘步进也无效                     |
| `step < 0`           | 取绝对值（assert + 自动修正）                                |
| `decimals < 0`       | clamp 到 0                                                  |
| `decimals > 6`       | clamp 到 6                                                  |
| `value < min`        | 钳位到 min                                                  |
| `value > max`        | 钳位到 max                                                  |
| `min = -∞, max = +∞` | 无范围限制，但仍按 step 量化                                |
| 编辑器小数超精度     | 四舍五入到 decimals 位                                      |

## 3. 类设计

### 3.1 NumericUpDown 类

NumericUpDown **继承自 EditBox**（类比 ComboBox 继承 EditBox），在右侧 margin 区域绘制 ▲/▼ 三角形用于步进，通过 `handleEvent` 拦截箭头区域点击和键盘事件，其余文本操作全部由 EditBox 基类处理。

```cpp
#ifndef NumericUpDownH
#define NumericUpDownH

#include <functional>
#include "SColor.h"
#include "EditBox.h"

class NumericUpDown : public EditBox {
    friend class NumericUpDownBuilder;
public:
    using OnValueChangedHandler =
        std::function<void(shared_ptr<NumericUpDown>, double newValue)>;

private:
    // ── 数据 ──
    double m_minValue;
    double m_maxValue;
    double m_step;
    double m_pageStep;
    double m_value;
    double m_committedValue;
    int    m_decimals;
    bool   m_readOnly;

    // ── 长按 ──
    bool   m_btnUpPressed;
    bool   m_btnDownPressed;
    double m_pressStartTime;
    double m_lastRepeatTime;

    // ── 箭头状态 ──
    bool   m_arrowUpHovered;
    bool   m_arrowDownHovered;

    // ── 视觉 ──
    float  m_buttonWidth;
    SColor m_arrowColor;
    SColor m_arrowHoverColor;
    SColor m_arrowPressColor;

    // ── 脏矩形 ──
    SRect  m_lastRect;

    // ── 回调 ──
    OnValueChangedHandler m_onValueChanged;

public:
    NumericUpDown(Control* parent, const SRect& rect,
                  float xScale = 1.0f, float yScale = 1.0f);
    ~NumericUpDown() override;

    void create() override;
    void update() override;
    void draw() override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void   setValue(double val);
    double getValue() const { return m_value; }
    void   setRange(double minVal, double maxVal);
    std::pair<double, double> getRange() const { return {m_minValue, m_maxValue}; }
    void   setStep(double step);
    double getStep() const { return m_step; }
    void   setPageStep(double ps);
    double getPageStep() const { return m_pageStep; }
    void   setDecimals(int n);
    int    getDecimals() const { return m_decimals; }
    void   setReadOnly(bool ro);
    bool   isReadOnly() const { return m_readOnly; }
    void   setButtonWidth(float w);
    float  getButtonWidth() const { return m_buttonWidth; }
    void   setArrowColor(SColor normal, SColor hover, SColor press);
    SColor getArrowColor() const { return m_arrowColor; }
    void   setOnValueChanged(OnValueChangedHandler handler);
    void   stepValue(int dir);

private:
    bool isInArrowArea(float x) const;
    bool isInUpArrow(float x, float y) const;
    bool isInDownArrow(float x, float y) const;
    void handleArrowPress(bool up);
    void handleArrowRelease();
    void handleKeyDownEvent(shared_ptr<Event> event);
    void handleRepeat();
    void setValueInternal(double val, bool fireCallback);
    void commitEditBoxText();
    double clampAndSnap(double val) const;
    string formatValue(double v) const;
    void onFocusLost() override;
};
```

### 3.2 NumericUpDownBuilder 类

```cpp
class NumericUpDownBuilder {
private:
    shared_ptr<NumericUpDown> m_ctl;
public:
    NumericUpDownBuilder(Control* parent, const SRect& rect,
                         float xScale = 1.0f, float yScale = 1.0f);
    NumericUpDownBuilder& setValue(double val);
    NumericUpDownBuilder& setRange(double minVal, double maxVal);
    NumericUpDownBuilder& setStep(double step);
    NumericUpDownBuilder& setPageStep(double ps);
    NumericUpDownBuilder& setDecimals(int n);
    NumericUpDownBuilder& setPlaceholder(const string& p);
    NumericUpDownBuilder& setReadOnly(bool ro);
    NumericUpDownBuilder& setButtonWidth(float w);
    NumericUpDownBuilder& setArrowColor(SColor normal, SColor hover, SColor press);
    NumericUpDownBuilder& setBackgroundStateColor(StateColor sc);
    NumericUpDownBuilder& setBorderStateColor(StateColor sc);
    NumericUpDownBuilder& setTextStateColor(StateColor sc);
    NumericUpDownBuilder& setFocusable(bool focusable);
    NumericUpDownBuilder& setFocusRingAlwaysVisible(bool visible);
    NumericUpDownBuilder& setFocusRingColor(SColor color);
    NumericUpDownBuilder& setId(int id);
    NumericUpDownBuilder& setOnValueChanged(NumericUpDown::OnValueChangedHandler cb);
    shared_ptr<NumericUpDown> build();
    operator shared_ptr<NumericUpDown>() { return build(); }
};
```

```cpp
#ifndef NumericUpDownH
#define NumericUpDownH

#include <functional>
#include "SColor.h"
#include "ControlBase.h"
#include "EditBox.h"

/// 按钮位置模式
enum class NumericUpDownButtonPosition {
    RightSide,        // 方案 A：右侧独立按钮区（默认，符合需求文档 §6）
## 4. 交互逻辑

NumericUpDown 继承 EditBox，事件处理采用 ComboBox 模式：先拦截箭头区域点击和键盘事件，其余回退到 EditBox::handleEvent(event)。

### 4.1 handleEvent

`cpp
bool NumericUpDown::handleEvent(shared_ptr<Event> event) {
    if (!m_enable || !m_visible) return false;

    // 只读模式：跳过交互但允许事件传播（由 KeyDown 块的聚焦检查决定消费）
    if (m_readOnly) {
        if (event->m_type == EventType::MouseDown ||
            event->m_type == EventType::KeyDown ||
            event->m_type == EventType::TextInput)
            return false;
    }

    // 箭头区点击（优先于 EditBox 的文本选中）
    if (event->m_type == EventType::MouseDown &&
        event->mouseButton.button == MouseButton::Left) {
        if (isContainsPoint(event->mouseButton.x, event->mouseButton.y)) {
            if (isInUpArrow(event->mouseButton.x, event->mouseButton.y)) {
                handleArrowPress(true); setFocused(true); return true;
            }
            if (isInDownArrow(event->mouseButton.x, event->mouseButton.y)) {
                handleArrowPress(false); setFocused(true); return true;
            }
        }
    }

    // 鼠标松开 → 停止长按
    if (event->m_type == EventType::MouseUp &&
        event->mouseButton.button == MouseButton::Left) {
        if (m_btnUpPressed || m_btnDownPressed) {
            handleArrowRelease(); return true;
        }
    }

    // 箭头区 hover
    if (event->m_type == EventType::MouseMove) {
        if (isContainsPoint(event->mousePos.x, event->mousePos.y)) {
            m_arrowUpHovered = isInUpArrow(event->mousePos.x, event->mousePos.y);
            m_arrowDownHovered = isInDownArrow(event->mousePos.x, event->mousePos.y);
        } else {
            m_arrowUpHovered = m_arrowDownHovered = false;
        }
    }

    // 键盘拦截（优先级高于 EditBox 的光标/剪贴板处理）
    if (event->m_type == EventType::KeyDown && getFocused()) {
        if (m_readOnly) return true;  // 只读聚焦控件：消费事件，不做操作
        switch (event->keyEvent.keycode) {
            case KeyCode::Up: case KeyCode::Plus: stepValue(+1); return true;
            case KeyCode::Down: case KeyCode::Minus: stepValue(-1); return true;
            case KeyCode::PageUp: setValue(m_value + m_pageStep); return true;
            case KeyCode::PageDown: setValue(m_value - m_pageStep); return true;
            case KeyCode::Home: setValue(m_minValue); return true;
            case KeyCode::End: setValue(m_maxValue); return true;
            case KeyCode::Return: commitEditBoxText(); return true;
            default: break;
        }
    }

    // 其余交给 EditBox 基类（光标移动、文本输入、剪贴板等）
    return EditBox::handleEvent(event);
}
`

### 4.2 箭头区检测

`cpp
bool NumericUpDown::isInArrowArea(float x) const {
    SRect dr = getDrawRect();
    float arrowStartX = dr.right() - m_buttonWidth * getScaleXX();
    return x >= arrowStartX && x <= dr.right();
}

bool NumericUpDown::isInUpArrow(float x, float y) const {
    if (!isInArrowArea(x)) return false;
    SRect dr = getDrawRect();
    return y < dr.top + dr.height / 2.0f;
}

bool NumericUpDown::isInDownArrow(float x, float y) const {
    if (!isInArrowArea(x)) return false;
    SRect dr = getDrawRect();
    return y >= dr.top + dr.height / 2.0f;
}
`

### 4.3 长按重复

`cpp
void NumericUpDown::update() {
    EditBox::update();
    handleRepeat();
}

void NumericUpDown::handleRepeat() {
    if (!m_btnUpPressed && !m_btnDownPressed) return;
    double now = Platform::GetTicks() / 1000.0;
    if (now - m_pressStartTime < 0.5) return;
    if (now - m_lastRepeatTime >= 0.1) {
        stepValue(m_btnUpPressed ? +1 : -1);
        m_lastRepeatTime = now;
    }
}
`

### 4.4 值操作

`cpp
void NumericUpDown::stepValue(int dir) {
    setValueInternal(m_value + dir * m_step, true);
}

void NumericUpDown::setValue(double val) {
    setValueInternal(val, true);
}

void NumericUpDown::setValueInternal(double val, bool fireCallback) {
    double oldCommitted = m_committedValue;
    m_value = clampAndSnap(val);
    EditBox::setText(formatValue(m_value));
    if (fireCallback) {
        m_committedValue = m_value;
        if (m_onValueChanged && m_committedValue != oldCommitted)
            m_onValueChanged(std::static_pointer_cast<NumericUpDown>(shared_from_this()), m_committedValue);
    }
}

void NumericUpDown::commitEditBoxText() {
    string text = EditBox::getText();
    if (text.empty()) { setValueInternal(m_minValue, true); return; }
    try {
        size_t pos = 0;
        double val = std::stod(text, &pos);
        if (pos != text.size() || std::isnan(val) || std::isinf(val)) throw std::invalid_argument("");
        setValueInternal(val, true);
    } catch (...) {
        EditBox::setText(formatValue(m_value));
    }
}
`

## 5. 渲染设计

### 5.1 绘制顺序

`cpp
void NumericUpDown::draw() {
    // 1. EditBox 基类绘制文本、光标、选中、背景、边框、焦点环
    EditBox::draw();

    // 2. 在右侧 margin 区域绘制 ▲/▼ 三角形
    SRect dr = getDrawRect();
    float sx = getScaleXX();
    float arrowRight = dr.right();
    float arrowLeft  = arrowRight - m_buttonWidth * sx;
    float arrowCenterX = (arrowLeft + arrowRight) / 2.0f;
    float centerY = dr.top + dr.height / 2.0f;

    float triH = min(m_buttonWidth * sx, dr.height) * 0.35f;
    float halfW = triH * 0.8f;

    // ▲ (向上)
    SColor upColor = m_btnUpPressed ? m_arrowPressColor
                   : m_arrowUpHovered ? m_arrowHoverColor : m_arrowColor;
    GET_RENDERDEVICE->setDrawColor(upColor);
    GET_RENDERDEVICE->drawTriangle(
        arrowCenterX, centerY - triH * 0.6f,
        arrowCenterX - halfW, centerY + triH * 0.4f,
        arrowCenterX + halfW, centerY + triH * 0.4f,
        upColor);

    // ▼ (向下)
    SColor downColor = m_btnDownPressed ? m_arrowPressColor
                      : m_arrowDownHovered ? m_arrowHoverColor : m_arrowColor;
    GET_RENDERDEVICE->setDrawColor(downColor);
    GET_RENDERDEVICE->drawTriangle(
        arrowCenterX, centerY + triH * 0.6f,
        arrowCenterX - halfW, centerY - triH * 0.4f,
        arrowCenterX + halfW, centerY - triH * 0.4f,
        downColor);
}
`

### 5.2 边距

构造时通过调整 EditBox 的右边距为箭头留出空间：

`cpp
NumericUpDown::NumericUpDown(Control* parent, const SRect& rect,
                             float xScale, float yScale)
    : EditBox(parent, rect, xScale, yScale)
    , m_minValue(0.0), m_maxValue(100.0), m_step(1.0)
    , m_pageStep(10.0), m_value(0.0), m_committedValue(0.0)
    , m_decimals(0), m_readOnly(false)
    , m_btnUpPressed(false), m_btnDownPressed(false)
    , m_pressStartTime(0.0), m_lastRepeatTime(0.0)
    , m_arrowUpHovered(false), m_arrowDownHovered(false)
    , m_buttonWidth(16.0f)
    , m_arrowColor(200, 200, 200, 255)
    , m_arrowHoverColor(255, 255, 255, 255)
    , m_arrowPressColor(180, 180, 180, 255)
    , m_lastRect(), m_onValueChanged(nullptr)
{
    m_fontSize = 14;
    m_margin.right += m_buttonWidth;  // 为右侧箭头保留空间
    setTransparent(true);
    setBorderVisible(false);
    setFocusable(true);
}
`

右边距 m_margin.right 减少 EditBox 的文本显示宽度，光标/选中/文字均不会进入箭头区域。与 ComboBox 的 m_margin.right += m_arrowWidth 完全相同的机制。

### 5.3 create

`cpp
void NumericUpDown::create() {
    EditBox::create();
    EditBox::setText(formatValue(m_value));
}
`

无需创建任何子控件。EditBox::create() 内部自动加载字体、初始化文本引擎。

## 6. 默认常量

```cpp
// ── 几何 ──
static const float NUMERICUPDOWN_BUTTON_WIDTH           = 28.0f;  // 默认箭头区宽度
static const float NUMERICUPDOWN_MIN_WIDTH_PADDING      = 10.0f;  // 文字区最小宽度

// ── 三角 ──
static const float NUMERICUPDOWN_TRIANGLE_HEIGHT_RATIO  = 0.22f;  // 三角高度 = 箭头区 * 比率
static const float NUMERICUPDOWN_TRIANGLE_WIDTH_RATIO   = 1.2f;   // 三角宽度 = 高度 * 比率（底 > 高）
static const float NUMERICUPDOWN_ARROW_GAP              = 3.0f;   // 上下三角间距

// ── 文字 ──
static const int   NUMERICUPDOWN_FONT_SIZE              = 14;
static const int   NUMERICUPDOWN_DECIMALS_MAX           = 6;      // 最大小数位数

// ── 长按重复 ──
static const double NUMERICUPDOWN_REPEAT_DELAY_SEC      = 0.5;    // 500ms 延迟
static const double NUMERICUPDOWN_REPEAT_INTERVAL_SEC   = 0.1;    // 100ms 间隔

// ── 默认范围/步长/精度 ──
static const double NUMERICUPDOWN_DEFAULT_STEP          = 1.0;
static const double NUMERICUPDOWN_DEFAULT_PAGESTEP      = 10.0;
static const int    NUMERICUPDOWN_DEFAULT_DECIMALS      = 0;

// ── 颜色 ──
static const SColor NUMERICUPDOWN_BG_COLOR(45, 45, 45, 255);           // 背景色
static const SColor NUMERICUPDOWN_ARROW_BG(55, 55, 55, 255);           // 箭头区背景
static const SColor NUMERICUPDOWN_ARROW_BG_HOVER(65, 65, 65, 255);
static const SColor NUMERICUPDOWN_ARROW_BG_PRESS(75, 75, 75, 255);
static const SColor NUMERICUPDOWN_SEPARATOR_COLOR(40, 40, 40, 255);    // 分隔线
static const SColor NUMERICUPDOWN_ARROW_COLOR(180, 180, 180, 255);     // 三角色
static const SColor NUMERICUPDOWN_ARROW_HOVER_COLOR(255, 255, 255, 255);
static const SColor NUMERICUPDOWN_ARROW_PRESS_COLOR(220, 220, 220, 255);
```

## 7. JSON 布局支持

### 7.1 LayoutParser 注册

在 `src/LayoutParser.cpp::parseControl()` 的 if-else 链中新增：

```cpp
else if (type == "NumericUpDown")
    result = parseNumericUpDown(j, parent);
```

### 7.2 `parseNumericUpDown` 实现

```cpp
shared_ptr<NumericUpDown> LayoutParser::parseNumericUpDown(
    const json& j, Control* parent)
{
    SRect rect = parseRect(j["rect"]);
    auto nud = make_shared<NumericUpDown>(parent, rect);

    m_theme.applyCommonColors(nud, "numericupdown");
    parseCommonProperties(nud, j);

    // 控件特有属性
    if (j.contains("value"))
        nud->setValue(j["value"].get<double>());

    if (j.contains("range") && j["range"].is_object()) {
        const json& r = j["range"];
        double mn = r.value("min", 0.0);
        double mx = r.value("max", 100.0);
        nud->setRange(mn, mx);
    }

    if (j.contains("step"))
        nud->setStep(j["step"].get<double>());

    if (j.contains("pageStep"))
        nud->setPageStep(j["pageStep"].get<double>());

    if (j.contains("decimals"))
        nud->setDecimals(j["decimals"].get<int>());

    if (j.contains("placeholder"))
        nud->setPlaceholder(j["placeholder"].get<string>());

    if (j.contains("readOnly"))
        nud->setReadOnly(j["readOnly"].get<bool>());

    if (j.contains("buttonWidth"))
        nud->setButtonWidth(j["buttonWidth"].get<float>());

    // 事件
    parseEvents(nud, j);

    // ID 注册
    if (j.contains("id") && j["id"].is_string())
        m_controlsById[j["id"].get<string>()] = nud;

    nud->create();
    return nud;
}
```

### 7.3 `parseEvents` 扩展（与 Slider 一致）

```cpp
if (auto nud = dynamic_pointer_cast<NumericUpDown>(ctrl)) {
    if (events.contains("onValueChanged") && events["onValueChanged"].is_string()) {
        string handlerName = events["onValueChanged"].get<string>();
        auto it = m_handlers.find(handlerName);
        if (it != m_handlers.end()) {
            auto handler = it->second;
            nud->setOnValueChanged([handler](shared_ptr<Control>, double) {
                handler(nullptr);
            });
        }
    }
}
```

### 7.4 JSON 格式

```json
{
    "type": "NumericUpDown",
    "id": "posX",
    "rect": { "x": 10, "y": 10, "w": 120, "h": 24 },
    "value": 100,
    "range": { "min": 0, "max": 4096 },
    "step": 1,
    "pageStep": 10,
    "decimals": 0,
    "placeholder": "0",
    "readOnly": false,
    "buttonWidth": 16,
    "events": {
        "onValueChanged": "onPosXChanged"
    }
}
```

**完整示例**（用于 PropertyGrid）：

```json
{
    "type": "NumericUpDown",
    "id": "fontSize",
    "rect": { "x": 100, "y": 50, "w": 100, "h": 24 },
    "value": 14,
    "range": { "min": 6, "max": 72 },
    "step": 1,
    "decimals": 0,
    "events": {
        "onValueChanged": "onFontSizeChanged"
    }
}

{
    "type": "NumericUpDown",
    "id": "scale",
    "rect": { "x": 100, "y": 80, "w": 100, "h": 24 },
    "value": 1.0,
    "range": { "min": 0.1, "max": 10.0 },
    "step": 0.1,
    "decimals": 2,
    "events": {
        "onValueChanged": "onScaleChanged"
    }
}

{
    "type": "NumericUpDown",
    "id": "opacity",
    "rect": { "x": 100, "y": 110, "w": 100, "h": 24 },
    "value": 1.0,
    "range": { "min": 0.0, "max": 1.0 },
    "step": 0.05,
    "decimals": 2,
    "buttonWidth": 20,
    "events": {
        "onValueChanged": "onOpacityChanged"
    }
}
```

### 7.5 箭头区域设计

NumericUpDown **继承自 EditBox**，在右侧 margin 区域绘制 ▲/▼ 三角形。

**布局**：

```
┌──────────────────────────────────┬──────┐
│  EditBox 文本区                  │  ▲  │
│  (m_margin.right 留出空间)        │  ─  │  ← 水平分隔线
│                                  │  ▼  │
└──────────────────────────────────┴──────┘
                                   ↑ m_buttonWidth (默认 28px)
```

**实现**：

- 构造时 `m_margin.right += m_buttonWidth`，EditBox 的文本渲染自动避开箭头区
- `draw()`: `EditBox::draw()` 后，在右侧 margin 内绘制三角和分隔线
- `handleEvent`: 通过 `isInArrowArea(x)` + `isInUpArrow(x,y)` / `isInDownArrow(x,y)` 检测点击
- 三个层都不遮挡焦点环（箭头背景从右边缘内缩 3px）

## 8. 测试计划

### 8.1 测试矩阵

| 类别       | 测试项                     | 优先级 | 自动化方法                |
|------------|----------------------------|--------|---------------------------|
| 显示       | 初始值渲染                 | P0     | 单元 + 视觉               |
| 显示       | 不同 step/decimals 显示    | P0     | 单元 + 视觉               |
| 显示       | 2x 缩放                    | P0     | 视觉 + 坐标               |
| 步进       | 单击 ▲/▼                   | P0     | EventQueue 注入 + 断言    |
| 步进       | 长按 ▲                     | P0     | Platform::GetTicks + 断言 |
| 步进       | 长按 ▼                     | P0     | 同上                      |
| 键盘       | ↑/↓/+/−                    | P0     | KeyDown 事件              |
| 键盘       | PageUp/PageDown            | P1     | KeyDown 事件，验证 setPageStep(25) |
| 键盘       | Home/End                   | P1     | KeyDown 事件              |
| 范围       | 输入越界值                 | P0     | EditBox.setText + commit   |
| 范围       | 步进越界                   | P0     | stepValue + 断言          |
| 范围       | 无穷范围                   | P1     | setRange(-inf, +inf)      |
| 步长       | step=0.1                   | P0     | 单元                      |
| 步长       | step=10                    | P0     | 单元                      |
| 步长       | step=0（边界）             | P1     | 单元                      |
| 精度       | decimals=2                 | P0     | 单元                      |
| 精度       | decimals=6                 | P2     | 单元                      |
| 占位符     | 空值 → placeholder 显示    | P1     | 视觉 + EditBox 文本       |
| 只读       | setReadOnly(true)          | P2     | TextInput 事件被丢弃      |
| 只读       | 只读时按钮仍可用           | P2     | stepValue 仍工作          |
| 焦点       | Tab 焦点离开               | P0     | 单元                      |
| 焦点       | FocusLost 提交值           | P0     | 单元                      |
| 焦点       | 3 层焦点环                 | P1     | 视觉                      |
| 事件       | onValueChanged 触发次数    | P0     | 计数器 + 断言             |
| 事件       | 回调参数正确               | P0     | 计数器                    |
| JSON       | 全参数解析                 | P0     | 单元                      |
| JSON       | 缺省参数解析               | P0     | 单元                      |
| 内存       | 长按 1 小时不泄漏          | P2     | 内存检测                  |

### 8.2 测试用例详细代码（test_numericupdown.cpp）

```cpp
#include "AppCallbacks.h"
#include "MainWindow.h"
#include "NumericUpDown.h"
#include "Label.h"
#include "Button.h"
#include <cassert>

// ── 全局状态 ──
static int g_valueChangedCount = 0;
static double g_lastValue = 0;
static shared_ptr<Label> g_statusLabel;

void onValueChangedCounter(shared_ptr<NumericUpDown> nud, double val) {
    g_valueChangedCount++;
    g_lastValue = val;
    if (g_statusLabel) {
        g_statusLabel->setCaption("ValueChanged #" +
            std::to_string(g_valueChangedCount) +
            " = " + std::to_string(val));
    }
}

class NumericUpDownApp : public AppCallbacks {
public:
    void onInit() override {
        MAINWIN->setTitle("test_numericupdown");

        // ── 测试 1: 默认值显示 ──
        auto nud1 = NumericUpDownBuilder(BENCH.get(), {50, 50, 150, 24})
            .setValue(50)
            .setRange(0, 100)
            .setStep(1)
            .setOnValueChanged(onValueChangedCounter)
            .build();
        BENCH->addControl(nud1);

        // ── 测试 2: 浮点 step + decimals ──
        auto nud2 = NumericUpDownBuilder(BENCH.get(), {50, 90, 150, 24})
            .setValue(1.0)
            .setRange(0.1, 10.0)
            .setStep(0.1)
            .setDecimals(2)
            .build();
        BENCH->addControl(nud2);

        // ── 测试 2b: 用户自定义 step=0.2（每次按下 +0.2 / -0.2）──
        // 用例：透明度滑块编辑器，每次微调 0.2
        auto nud2b = NumericUpDownBuilder(BENCH.get(), {220, 90, 150, 24})
            .setValue(0.6)
            .setRange(0.0, 1.0)
            .setStep(0.2)         // ← 用户自定义步长
            .setDecimals(2)
            .setOnValueChanged([](shared_ptr<NumericUpDown>, double v) {
                printf("[nud2b] step=0.2 步进，当前值 = %.2f\n", v);
            })
            .build();
        BENCH->addControl(nud2b);

        // ── 测试 2c: 大 step=50 ──
        // 用例：数量编辑器，每次 ±50
        auto nud2c = NumericUpDownBuilder(BENCH.get(), {390, 90, 150, 24})
            .setValue(100)
            .setRange(0, 10000)
            .setStep(50)
            .setDecimals(0)
            .build();
        BENCH->addControl(nud2c);

        // ── 测试 3: 大 step ──
        auto nud3 = NumericUpDownBuilder(BENCH.get(), {50, 130, 150, 24})
            .setValue(50)
            .setRange(0, 1000)
            .setStep(10)
            .build();
        BENCH->addControl(nud3);

        // ── 测试 4: 负值范围 ──
        auto nud4 = NumericUpDownBuilder(BENCH.get(), {50, 170, 150, 24})
            .setValue(-50)
            .setRange(-100, 100)
            .setStep(5)
            .build();
        BENCH->addControl(nud4);

        // ── 测试 5: 高精度 decimals=4 ──
        auto nud5 = NumericUpDownBuilder(BENCH.get(), {50, 210, 150, 24})
            .setValue(3.14159)
            .setRange(0.0, 10.0)
            .setStep(0.0001)
            .setDecimals(4)
            .build();
        BENCH->addControl(nud5);

        // ── 测试 6: 占位符 ──
        auto nud6 = NumericUpDownBuilder(BENCH.get(), {50, 250, 150, 24})
            .setPlaceholder("Enter value...")
            .setRange(0, 100)
            .build();
        BENCH->addControl(nud6);

        // ── 测试 7: 只读 ──
        auto nud7 = NumericUpDownBuilder(BENCH.get(), {50, 290, 150, 24})
            .setValue(42)
            .setReadOnly(true)
            .build();
        BENCH->addControl(nud7);

        // ── 测试 8: 无穷范围 ──
        auto nud8 = NumericUpDownBuilder(BENCH.get(), {50, 330, 150, 24})
            .setValue(0)
            .setRange(-std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::infinity())
            .setStep(0.001)
            .setDecimals(3)
            .build();
        BENCH->addControl(nud8);

        // ── 测试 9: 大按钮宽度 ──
        auto nud9 = NumericUpDownBuilder(BENCH.get(), {50, 370, 200, 24})
            .setValue(50)
            .setButtonWidth(24)
            .build();
        BENCH->addControl(nud9);

        // ── 测试 10: 2x 缩放 ──
        auto nud10 = NumericUpDownBuilder(BENCH.get(), {300, 50, 150, 24}, 2.0f, 2.0f)
            .setValue(100)
            .setRange(0, 200)
            .setStep(5)
            .build();
        BENCH->addControl(nud10);

        // ── 测试 11: 自定义颜色 ──
        auto nud11 = NumericUpDownBuilder(BENCH.get(), {50, 410, 150, 24})
            .setValue(50)
            .setButtonColor({70, 70, 70, 255}, {100, 100, 100, 255}, {130, 130, 130, 255})
            .setArrowColor({255, 255, 0, 255})    // 黄色箭头
            .build();
        BENCH->addControl(nud11);

        // ── 测试 12: pageStep 验证 ──
        auto nud12 = NumericUpDownBuilder(BENCH.get(), {50, 450, 150, 24})
            .setValue(50)
            .setRange(0, 1000)
            .setStep(1)
            .setPageStep(25)     // PageUp=+25, PageDown=-25
            .build();
        BENCH->addControl(nud12);

        // ── 状态标签 ──
        g_statusLabel = LabelBuilder(BENCH.get(), {50, 500, 400, 24})
            .setCaption("Click/Type to test")
            .build();
        BENCH->addControl(g_statusLabel);

        // ── 单元测试面板 ──
        g_statusLabel->setCaption("Ready: nud1@nud12 test, see asserts in console");
    }

    void onUpdate() override {
        // 长按测试可在这里加（依赖 mouseDown 状态）
    }

    void onRender() override {
        BENCH->draw();
    }

    void onQuit() override {
        // 验证最终状态
        printf("[test_numericupdown] Final valueChanged count = %d\n",
               g_valueChangedCount);
    }
};

int main(int argc, char** argv) {
    NumericUpDownApp app;
    MAINWIN->run(&app);
    return 0;
}
```

### 8.3 自动化断言（在测试代码中嵌入）

```cpp
// ── 单元测试断言（可通过 UI 按钮或初始化时调用） ──
void runNumericUpDownUnitTests() {
    g_valueChangedCount = 0;

    // Test 1: setValue + getValue
    {
        auto nud = NumericUpDownBuilder(nullptr, {0, 0, 100, 24}).build();
        nud->setValue(50);
        assert(nud->getValue() == 50);
        printf("[Test 1] PASS: setValue/getValue\n");
    }

    // Test 2: 范围钳位
    {
        auto nud = NumericUpDownBuilder(nullptr, {0, 0, 100, 24})
            .setRange(0, 100).build();
        nud->setValue(999);
        assert(nud->getValue() == 100);
        nud->setValue(-100);
        assert(nud->getValue() == 0);
        printf("[Test 2] PASS: range clamp\n");
    }

    // Test 3: step 量化
    {
        auto nud = NumericUpDownBuilder(nullptr, {0, 0, 100, 24})
            .setRange(0, 10).setStep(0.5).build();
        nud->setValue(3.7);
        assert(std::abs(nud->getValue() - 3.5) < 1e-9);  // 量化到 3.5
        printf("[Test 3] PASS: step snap\n");
    }

    // Test 4: 小数精度
    {
        auto nud = NumericUpDownBuilder(nullptr, {0, 0, 100, 24})
            .setRange(0, 10).setStep(0.01).setDecimals(2).build();
        nud->setValue(3.14159);
        assert(std::abs(nud->getValue() - 3.14) < 1e-9);
        printf("[Test 4] PASS: decimals rounding\n");
    }

    // Test 5: callback
    {
        auto nud = NumericUpDownBuilder(nullptr, {0, 0, 100, 24})
            .setValue(0)
            .setRange(0, 100)
            .setOnValueChanged([](shared_ptr<NumericUpDown>, double v) {
                g_lastValue = v;
            })
            .build();
        nud->setValue(50);
        assert(g_lastValue == 50);
        printf("[Test 5] PASS: callback\n");
    }

    // Test 6: 回调去重（值不变时不触发）
    {
        g_valueChangedCount = 0;
        auto nud = NumericUpDownBuilder(nullptr, {0, 0, 100, 24})
            .setValue(50)
            .setRange(0, 100)
            .setOnValueChanged(onValueChangedCounter)
            .build();
        nud->setValue(50);  // 相同值
        assert(g_valueChangedCount == 0);
        printf("[Test 6] PASS: callback dedup\n");
    }

    // Test 7: 无穷范围
    {
        auto nud = NumericUpDownBuilder(nullptr, {0, 0, 100, 24})
            .setRange(-std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::infinity())
            .setStep(1).build();
        nud->setValue(1e9);
        assert(nud->getValue() == 1e9);
        printf("[Test 7] PASS: infinity range\n");
    }

    // Test 8: 格式化
    {
        auto nud = NumericUpDownBuilder(nullptr, {0, 0, 100, 24})
            .setValue(3.14)
            .setRange(0, 10)
            .setDecimals(2).build();
        // 直接验证 EditBox 文本（内部访问）
        assert(nud->getEditBox()->getText() == "3.14");
        printf("[Test 8] PASS: format\n");
    }

    printf("=== All NumericUpDown unit tests PASSED ===\n");
}
```

### 8.4 长按测试专用代码

```cpp
// 在测试面板加一个 "Run Long-Press Test" 按钮
auto longPressBtn = ButtonBuilder(BENCH.get(), {50, 540, 150, 30})
    .setCaption("Long-Press Test")
    .setOnClick([](shared_ptr<Button>) {
        // 模拟长按：注入 MouseDown，1.5 秒后注入 MouseUp
        auto nud = BENCH->findControlById("longPressTest");
        // ... 注入事件序列 ...
    })
    .build();
```

### 8.5 JSON 测试用例（test_layout.json 追加）

```json
{
    "type": "NumericUpDown",
    "id": "jsonTest1",
    "rect": { "x": 50, "y": 580, "w": 150, "h": 24 },
    "value": 50,
    "range": { "min": 0, "max": 100 },
    "step": 1,
    "events": { "onValueChanged": "onJsonTest1Changed" }
},
{
    "type": "NumericUpDown",
    "id": "jsonTest2_float",
    "rect": { "x": 50, "y": 610, "w": 150, "h": 24 },
    "value": 1.0,
    "range": { "min": 0.0, "max": 5.0 },
    "step": 0.1,
    "decimals": 2
},
{
    "type": "NumericUpDown",
    "id": "jsonTest3_defaults",
    "rect": { "x": 50, "y": 640, "w": 150, "h": 24 }
}
```

### 8.6 C ABI 集成测试

#### 8.6.1 新建独立测试 `test_numericupdown_cabi.cpp`

完整代码见 §8.4.2。**验证 5 个控件场景**：

| 控件 ID         | step  | decimals | readOnly | buttonPosition   | 验证点 |
|-----------------|-------|----------|----------|------------------|--------|
| nudInteger      | 1     | 0        | false    | RightSide（默认）| 整数步进回调 |
| nudFloat02      | 0.2   | 2        | false    | RightSide        | 浮点 step 显示与回调 |
| nudBigStep      | 50    | 0        | false    | RightSide        | 大 step 不连续值 |
| nudReadOnly     | 1     | 0        | **true** | RightSide        | 只读模式按钮仍可用 |
| nudInline       | 0.01  | 2        | false    | **InlineEditBox**| 运行时切换模式 |

**测试通过标准**：

```c
// 断言 1：5 个控件全部成功创建（JSON 解析通过）
// 断言 2：运行时 SetNumericUpDownValue 立即生效
// 断言 3：onValueChanged 回调在每次步进/输入时触发
// 断言 4：nudInline 运行时切换 buttonPosition 1 (InlineEditBox) 视觉变化
// 断言 5：Quit 按钮关闭程序（参考 test_combobox_cabi 模式）
```

#### 8.6.2 追加到 `test_fromsource_cabi.cpp`（最小集成）

仿照 Slider 在 test_fromsource_cabi.cpp 中的写法（第 50-53、95-98、188-191、264-266、354-356 行），添加 NumericUpDown 的最小集成：

```c
// 在 test_fromsource_cabi.cpp 顶部追加 typedef（同 §8.4.1）

// 在 createAllControls() 中追加：
g_nudHandle = uiCreateNumericUpDown(20, 510, 200, 28);
uiSetNumericUpDownRange(g_nudHandle, 0.0, 100.0);
uiSetNumericUpDownStep(g_nudHandle, 1.0);
uiSetNumericUpDownValue(g_nudHandle, 50.0);
uiSetOnNumericUpDownValueChanged(g_nudHandle, onNudValueChanged, nullptr);

// 在 doFrame() 末尾追加：
double nv = uiGetNumericUpDownValue(g_nudHandle);
printf("NumericUpDown: %.2f\r", nv);
```

仅添加 1 个 NumericUpDown 用于**最小可行性验证**，不重复 test_numericupdown_cabi 的完整场景。

#### 8.6.3 三后端构建验证

```batch
build_scripts\build.bat sdl3    # 编译 SDL3 后端，输出 build\sdl3\test\Debug\test_numericupdown_cabi.exe
build_scripts\build.bat sfml    # 编译 SFML 后端
build_scripts\build.bat raylib  # 编译 Raylib 后端

build_scripts\build.bat sdl3 dll    # DLL 模式 + test_numericupdown_cabi.exe
```

DLL 模式下 5 个 NumericUpDown 通过 `UICornerstone.dll` C ABI 调用，验证：
- 工厂函数 `UICornerstone_CreateNumericUpDown`
- 12 个 setter/getter（含 pageStep）
- `buttonPosition` 运行时切换（`UICornerstone_SetNumericUpDownButtonPosition`）
- `onValueChanged` 回调签名 `void(*)(void*, double)`

## 9. 实现阶段划分

### 9.1 文件清单

| 文件                                  | 状态   | 内容                                | 预估行数 |
|---------------------------------------|--------|-------------------------------------|----------|
| `include/NumericUpDown.h`             | 新建   | 类声明 + Builder + buttonPosition enum | +220   |
| `src/NumericUpDown.cpp`               | 新建   | 完整实现（含两种 buttonPosition 模式） | +600   |
| `include/ConstDef.h`                  | 修改   | NUMERICUPDOWN_* 常量                | +20      |
| `src/ConstDef.cpp`                    | 修改   | 常量默认值                          | +20      |
| `src/LayoutParser.cpp`                | 修改   | parseNumericUpDown + buttonPosition JSON | +60  |
| `include/UICornerstoneAPI.h`          | 修改   | 12 个 C ABI 函数声明                | +55      |
| `src/UICornerstoneAPI.cpp`            | 修改   | C ABI 实现（含 buttonPosition setter） | +110   |
| `test/test_numericupdown.cpp`         | 新建   | 标准 C++ 测试 + 12 控件 + 单元断言  | +280     |
| **`test/test_numericupdown_cabi.cpp`** | **新建** | **独立 fromsource C ABI 测试**（5 控件场景）| **+250** |
| `test/test_fromsource_cabi.cpp`       | 修改   | 追加 NumericUpDown 最小集成（~20 行）| +20      |
| `test/CMakeLists.txt`                 | 修改   | 注册 test_numericupdown + test_numericupdown_cabi | +10 |
| `CMakeLists.txt`                      | 修改   | test_binary 列表                    | +1       |
| `layouts/test_layout.json`            | 修改   | 3 个 JSON 测试用例                  | +20      |
| `README.md`                           | 修改   | 控件列表                            | +2       |
| `doc/Build_Guide.md`                  | 修改   | 测试表（添加 test_numericupdown_cabi）| +2       |
| `doc/NumericUpDown_Design.md`         | 新建   | 本文档                              | (本文档) |
| `AGENTS.md`                           | 修改   | session 记录                        | +10      |
| `doc/UICornerstone_DLL_Design.md`     | 修改   | C ABI 列表（追加 12 个函数）         | +10      |

**总计**：约 **1665 行**（含标准测试 280 行 + C ABI 测试 250 行）

### 9.2 实现阶段

| 阶段 | 内容                                            | 文件                                      | 预估工作量 |
|------|-------------------------------------------------|-------------------------------------------|------------|
| 1    | 头文件骨架 + 类声明 + Builder + buttonPosition enum | `include/NumericUpDown.h`           | 2h         |
| 2    | 构造/析构 + create + layoutButtons + draw 骨架 | `src/NumericUpDown.cpp`                   | 3h         |
| 3    | handleEvent (Mouse + Key + TextInput)           | `src/NumericUpDown.cpp`                   | 3h         |
| 4    | stepValue + clampAndSnap + formatValue          | `src/NumericUpDown.cpp`                   | 2h         |
| 5    | 长按重复 update()                                | `src/NumericUpDown.cpp`                   | 1h         |
| 6    | 焦点管理 + Watcher                              | `src/NumericUpDown.cpp`                   | 1h         |
| 7    | 缩放感知 setRect + onScaleChanged               | `src/NumericUpDown.cpp`                   | 1h         |
| 8    | InlineEditBox 模式分支（hitRect/drawRect 差异化）| `src/NumericUpDown.cpp`                   | 2h         |
| 9    | 单元测试 + test_numericupdown.cpp 框架          | `test/test_numericupdown.cpp`             | 2h         |
| 10   | 三后端编译验证 (SDL3/SFML/Raylib)                | —                                         | 1h         |
| 11   | JSON 布局支持（含 buttonPosition 字段）         | `src/LayoutParser.cpp`                    | 2h         |
| 12   | ConstDef 常量 + 测试用例补充                    | `include/ConstDef.h`, `test/*.cpp`        | 1h         |
| 13   | C ABI 12 个函数（含 pageStep + buttonPosition setter） | `include/UICornerstoneAPI.h/.cpp`         | 2h         |
| 14   | **新建 test_numericupdown_cabi.cpp**（5 控件场景） | `test/test_numericupdown_cabi.cpp`     | 2h         |
| 15   | **test_fromsource_cabi.cpp 追加 NumericUpDown 最小集成** | `test/test_fromsource_cabi.cpp` | 0.5h       |
| 16   | DLL 三后端 fromsource 验证（test_numericupdown_cabi 三后端跑通） | `test/test_numericupdown_cabi.exe` | 1h |
| 17   | 文档与 AGENTS.md 更新                           | 全部 `.md`                                | 1h         |

**总计**：约 **28.5 小时**（含验证）

### 9.3 风险与回滚

- **EditBox 嵌入耦合**：若 EditBox 行为变更（如 2026-07-14 焦点环变更），需同步更新 NumericUpDown。**缓解**：NumericUpDown 通过 `getEditBox()` 公开访问，便于单独维护。
- **缩放感知复杂度**：参考 Slider 的 `getThumbRect()` 经验，避免双重缩放。**缓解**：所有内部 hitRect/drawRect 使用未缩放控件坐标。
- **长按焦点丢失**：若用户长按时点击其他控件，`m_btnUpPressed` 必须清零。**缓解**：`onFocusLost()` 中显式清零 + Watcher 检测外部点击。

## 10. 边界与约束

| 约束                         | 说明                                                                |
|------------------------------|---------------------------------------------------------------------|
| 仅支持水平方向               | 文档需求 §6 未指定垂直；如未来需要垂直，可通过 `SliderStyle` 扩展   |
| EditBox 嵌入，不单独暴露     | EditBox 是 NumericUpDown 的内部实现细节，不可作为独立 JSON 控件      |
| 单 NumericUpDown 单一焦点    | Tab 进出整个控件，不进入内部 EditBox                                 |
| 长按焦点不可被抢占           | 长按期间强制 `setFocused(true)`，需在 Watcher 中保持焦点             |
| C ABI 不支持 builder 链式    | C ABI 仅暴露关键 setter（值/范围/步长/精度/回调），不暴露 builder   |
| 浮点 step 必须 > 0           | step == 0 时 assert + 警告；step < 0 取绝对值                       |
| decimals ∈ [0, 6]            | 0 表示整数；超过 6 精度意义不大                                     |
| 范围 ∞ 时无钳位              | std::clamp 对 NaN 行为未定义，需先 std::isnan 守卫                  |
| 三后端 parity                | SDL3/SFML/Raylib 必须行为一致（EventQueue、RenderDevice、文本渲染） |

## 11. 关键实现注意事项

### 11.1 坐标空间一致性

```
m_rect          ── 未缩放，控件相对坐标  （setRect 参数原样）
getDrawRect()   ── 已缩放，绝对屏幕坐标  （含 getScaleXX/Y() 倍率）
hitRect/drawRect ── 未缩放，控件坐标    （hit-test 与绘制坐标分离）
EditBox.setRect ── 未缩放，控件坐标    （EditBox 内部再次缩放）
measureText     ── 已缩放像素          （需除以 scale 转未缩放）
```

### 11.2 缩放陷阱

- **不要传已缩放 rect 给 EditBox**：NumericUpDown 的 `m_rect` 是未缩放，`layoutButtons()` 计算的 `ebRect = {0, 0, totalW - halfBW, totalH}` 也是未缩放。EditBox 内部会应用 `getScaleXX/Y()`。错误做法：传 `{0, 0, (totalW-halfBW)*sx, totalH*sy}`。
- **hitRect 用未缩放控件坐标**：因为 MouseDown 事件的坐标是已缩放屏幕坐标，需在 `handleMouseDown` 中转换为控件坐标再比较，或者 `hitRect` 改为已缩放绝对坐标。**推荐**：保持 hitRect 未缩放，hit-test 前将 mouse 坐标转为未缩放（`(mouse - getAbsoluteOrigin()) / scale`）。
- **三角形顶点缩放**：`drawTriangle` 接受 SPoint 已是已缩放像素坐标，所以 `drawButton` 中计算三角形顶点时必须乘以 `sx/sy`。

### 11.3 长按生命周期

```
MouseDown in btnUp
  ↓
m_btnUpPressed = true
m_pressStartTime = now
stepValue(+1)              ← 立即步进
setFocused(true)
  ↓
[每帧 update() 检测]
  if (now - pressStartTime >= 500ms) && (now - lastRepeatTime >= 100ms)
    stepValue(+1)
    lastRepeatTime = now
  ↓
MouseUp anywhere
  ↓
m_btnUpPressed = false
m_btnUp.pressed = false
  ↓
[FocusLost / 外部 MouseDown / 关闭程序]
  ↓
onFocusLost() 中显式清零（防御性）
```

### 11.4 回调去重

```cpp
void NumericUpDown::setValueInternal(double val, bool fireCallback) {
    double oldCommitted = m_committedValue;
    m_value = clampAndSnap(val);
    EditBox::setText(formatValue(m_value));

    if (fireCallback) {
        m_committedValue = m_value;
        // 仅当值真正变化时才触发回调，避免循环触发
        if (m_onValueChanged && m_committedValue != oldCommitted) {
            m_onValueChanged(getThis(), m_committedValue);
        }
    }
}
```

### 11.5 三后端兼容性

| 后端      | 风险点                       | 缓解                                          |
|-----------|------------------------------|-----------------------------------------------|
| SDL3      | TTF 字体支持                  | 复用现有 EditBox 字体加载                      |
| SDL3      | Focus 焦点环 3 层             | 沿用 2026-07-07 优化                           |
| SFML      | EditBox 中文输入              | 复用 SFML TextRenderer 缓存                    |
| SFML      | drawTriangle winding         | SFML 已自动修正 CW/CCW                         |
| Raylib    | 中文显示 ?                    | ensureFontCodepoints 在 EditBox 内部已处理      |
| Raylib    | KeyUp 缺失导致长按不停止      | Slider 已修复 fillKeyUpEvent，NumericUpDown 复用 |
| Raylib    | 缩放下 hit-test 偏差          | 与 Slider 同样用未缩放 hitRect + 坐标转换      |

### 11.6 C ABI 句柄转换

参考 ColorPicker 的虚拟继承修复（`static_cast<Control*>(voidPtr)`）：

```cpp
UIControlHandle UICornerstone_CreateNumericUpDown(float x, float y, float w, float h) {
    auto nud = make_shared<NumericUpDown>(BENCH.get(), {x, y, w, h});
    nud->create();
    return reinterpret_cast<UIControlHandle>(static_cast<Control*>(nud.get()));
}

void UICornerstone_SetNumericUpDownValue(UIControlHandle ctl, double val) {
    auto nud = static_cast<NumericUpDown*>(
        reinterpret_cast<Control*>(ctl));
    if (nud) nud->setValue(val);
}
```

### 11.7 与 ComboBox/Dialog 弹窗模式对比

NumericUpDown 是 **单层扁平控件**（不弹窗），与 ComboBox/Dialog 的弹窗模式不同：
- ComboBox 的 ListPanel 是独立 `Popup` 节点，Tab 焦点环进入 ListPanel
- NumericUpDown 的 EditBox 是内部成员，对外不可见，Tab 直接离开整个控件

实现上不需要 Popup/FocusBoundary 机制，简化了交互。

## 12. C ABI

### 12.1 工厂函数

```c
UIControlHandle UICornerstone_CreateNumericUpDown(
    float x, float y, float w, float h);
```

### 12.2 值/范围/步长/精度 setter/getter

```c
void   UICornerstone_SetNumericUpDownValue(UIControlHandle ctl, double val);
double UICornerstone_GetNumericUpDownValue(UIControlHandle ctl);

void   UICornerstone_SetNumericUpDownRange(UIControlHandle ctl, double min, double max);
void   UICornerstone_SetNumericUpDownStep(UIControlHandle ctl, double step);
void   UICornerstone_SetNumericUpDownPageStep(UIControlHandle ctl, double ps);
void   UICornerstone_SetNumericUpDownDecimals(UIControlHandle ctl, int decimals);

void   UICornerstone_SetNumericUpDownPlaceholder(UIControlHandle ctl, const char* placeholder);
void   UICornerstone_SetNumericUpDownReadOnly(UIControlHandle ctl, bool readOnly);
void   UICornerstone_SetNumericUpDownButtonWidth(UIControlHandle ctl, float width);
```

### 12.3 事件回调

```c
typedef void (*UINumericUpDownValueChangedCallback)(void* userData, double newValue);

void UICornerstone_SetOnNumericUpDownValueChanged(
    UIControlHandle ctl,
    UINumericUpDownValueChangedCallback cb,
    void* userData);
```

### 12.4 C ABI 使用示例

#### 12.4.1 简单调用（test_fromsource_cabi.cpp 中追加）

参考 `test_fromsource_cabi.cpp` 中 Slider 的写法：

```c
// 在 test_fromsource_cabi.cpp 中追加 NumericUpDown 句柄
typedef void* (*UICreateNumericUpDownFn)(float, float, float, float);
typedef double (*UIGetNumericUpDownValueFn)(void*);
typedef void   (*UISetNumericUpDownValueFn)(void*, double);
typedef void   (*UISetNumericUpDownRangeFn)(void*, double, double);
typedef void   (*UISetNumericUpDownStepFn)(void*, double);
typedef void   (*UISetNumericUpDownPageStepFn)(void*, double);
typedef void   (*UISetNumericUpDownDecimalsFn)(void*, int);
typedef void   (*UISetOnNumericUpDownValueChangedFn)(void*, void (*)(void*, double), void*);

static UICreateNumericUpDownFn             uiCreateNumericUpDown        = nullptr;
static UIGetNumericUpDownValueFn           uiGetNumericUpDownValue      = nullptr;
static UISetNumericUpDownValueFn           uiSetNumericUpDownValue      = nullptr;
static UISetNumericUpDownRangeFn           uiSetNumericUpDownRange      = nullptr;
static UISetNumericUpDownStepFn            uiSetNumericUpDownStep       = nullptr;
static UISetNumericUpDownPageStepFn        uiSetNumericUpDownPageStep   = nullptr;
static UISetNumericUpDownDecimalsFn        uiSetNumericUpDownDecimals   = nullptr;
static UISetOnNumericUpDownValueChangedFn  uiSetOnNumericUpDownValueChanged = nullptr;

static void* g_nudHandle = nullptr;

// 在 loadAllProcs() 中追加：
RESOLVE(CreateNumericUpDown);
RESOLVE(GetNumericUpDownValue);
RESOLVE(SetNumericUpDownValue);
RESOLVE(SetNumericUpDownRange);
RESOLVE(SetNumericUpDownStep);
RESOLVE(SetNumericUpDownPageStep);
RESOLVE(SetNumericUpDownDecimals);
RESOLVE(SetOnNumericUpDownValueChanged);

// 创建 + 设置
g_nudHandle = uiCreateNumericUpDown(20, 510, 200, 28);
uiSetNumericUpDownRange(g_nudHandle, 0.0, 100.0);
uiSetNumericUpDownStep(g_nudHandle, 1.0);
uiSetNumericUpDownDecimals(g_nudHandle, 0);
uiSetNumericUpDownValue(g_nudHandle, 50.0);

// 回调
static void onNudValueChanged(void* ctl, double newValue) {
    printf("NumericUpDown value = %.2f\n", newValue);
}
uiSetOnNumericUpDownValueChanged(g_nudHandle, onNudValueChanged, nullptr);

// 帧循环中读取
double v = uiGetNumericUpDownValue(g_nudHandle);
```

#### 12.4.2 独立 fromsource 测试（test_numericupdown_cabi.cpp，新建）

**完整文件**（参考 `test_combobox_cabi.cpp` 结构）：

```cpp
// =========================================================================
// test_numericupdown_cabi.cpp -- single fromsource C ABI test for NumericUpDown (all backends)
// Backend name provided via -DBACKEND_SHORT_NAME / -DBACKEND_DISPLAY_NAME
// =========================================================================

#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../../include/UICornerstoneAPI.h"

extern "C" UIBackendCallbacks* GetUIBackendCallbacks(void);

// ===== C ABI function pointer types =====
typedef int   (*UIInitFn)(void*);
typedef void  (*UISetViewportFn)(float,float,float,float);
typedef void  (*UIProcessEventsFn)(void);
typedef void  (*UIUpdateFn)(double);
typedef void  (*UIClearFn)(void);
typedef void  (*UIRenderFn)(void);
typedef void  (*UIPresentFn)(void);
typedef int   (*UIIsQuitFn)(void);
typedef void  (*UIShutdownFn)(void);
typedef int   (*UILoadLayoutFn)(const char*);
typedef void* (*UIFindControlFn)(const char*);
typedef void  (*UIRegisterActionFn)(const char*,void(*)(void*,void*),void*);

// ── NumericUpDown C ABI ──
typedef void* (*UICreateNumericUpDownFn)(float,float,float,float);
typedef double (*UIGetNumericUpDownValueFn)(void*);
typedef void   (*UISetNumericUpDownValueFn)(void*, double);
typedef void   (*UISetNumericUpDownRangeFn)(void*, double, double);
typedef void   (*UISetNumericUpDownStepFn)(void*, double);
typedef void   (*UISetNumericUpDownPageStepFn)(void*, double);
typedef void   (*UISetNumericUpDownDecimalsFn)(void*, int);
typedef void   (*UISetNumericUpDownReadOnlyFn)(void*, int);
typedef void   (*UISetNumericUpDownPlaceholderFn)(void*, const char*);
typedef void   (*UISetNumericUpDownButtonPositionFn)(void*, int);  // 0=RightSide 1=InlineEditBox
typedef void   (*UISetNumericUpDownButtonWidthFn)(void*, float);
typedef void   (*UISetOnNumericUpDownValueChangedFn)(void*, void (*)(void*, double), void*);

static UIInitFn             uiInit                       = nullptr;
static UISetViewportFn      uiSetViewport                = nullptr;
static UIProcessEventsFn    uiProcessEvents              = nullptr;
static UIUpdateFn           uiUpdate                     = nullptr;
static UIClearFn            uiClear                      = nullptr;
static UIRenderFn           uiRender                     = nullptr;
static UIPresentFn          uiPresent                    = nullptr;
static UIIsQuitFn           uiIsQuitRequested            = nullptr;
static UIShutdownFn         uiShutdown                   = nullptr;
static UILoadLayoutFn       uiLoadLayout                 = nullptr;
static UIFindControlFn      uiFindControl                = nullptr;
static UIRegisterActionFn   uiRegisterAction             = nullptr;

static UICreateNumericUpDownFn              uiCreateNumericUpDown              = nullptr;
static UIGetNumericUpDownValueFn            uiGetNumericUpDownValue            = nullptr;
static UISetNumericUpDownValueFn            uiSetNumericUpDownValue            = nullptr;
static UISetNumericUpDownRangeFn            uiSetNumericUpDownRange            = nullptr;
static UISetNumericUpDownStepFn             uiSetNumericUpDownStep             = nullptr;
static UISetNumericUpDownPageStepFn         uiSetNumericUpDownPageStep         = nullptr;
static UISetNumericUpDownDecimalsFn         uiSetNumericUpDownDecimals         = nullptr;
static UISetNumericUpDownReadOnlyFn         uiSetNumericUpDownReadOnly         = nullptr;
static UISetNumericUpDownPlaceholderFn      uiSetNumericUpDownPlaceholder      = nullptr;
static UISetNumericUpDownButtonPositionFn   uiSetNumericUpDownButtonPosition   = nullptr;
static UISetNumericUpDownButtonWidthFn      uiSetNumericUpDownButtonWidth      = nullptr;
static UISetOnNumericUpDownValueChangedFn   uiSetOnNumericUpDownValueChanged   = nullptr;

static HMODULE g_uiDll = nullptr;

// ===== 回调 =====
static char g_statusInfo[256] = "NumericUpDown C ABI Test";

static void onValueChanged(void* ctl, double newValue) {
    snprintf(g_statusInfo, sizeof(g_statusInfo),
        "ValueChanged: nud=%p value=%.2f", ctl, newValue);
    printf("%s\n", g_statusInfo);
    void* lbl = uiFindControl("lblStatus");
    if (lbl) {
        char txt[128];
        snprintf(txt, sizeof(txt), "Last: %.2f", newValue);
        // 调用通用 SetText，假设 UICornerstone_SetText 接受控件 + 字符串
        extern void UICornerstone_SetText(void*, const char*);  // 已存在
        UICornerstone_SetText(lbl, txt);
    }
}

static void onQuitClicked(void* ctl, void* user) {
    (void)ctl; (void)user;
    printf("[test] Quit button clicked\n");
    extern void UICornerstone_RequestQuit(void);  // 已存在
    UICornerstone_RequestQuit();
}

static void loadAllProcs(HMODULE dll) {
#define RESOLVE(name) \
    *(void**)&ui##name = GetProcAddress(dll, "UICornerstone_" #name)

    // ── Core ──
    RESOLVE(Init);
    RESOLVE(SetViewport);
    RESOLVE(ProcessEvents);
    RESOLVE(Update);
    RESOLVE(Clear);
    RESOLVE(Render);
    RESOLVE(Present);
    RESOLVE(IsQuitRequested);
    RESOLVE(Shutdown);
    RESOLVE(LoadLayout);
    RESOLVE(FindControl);
    RESOLVE(RegisterAction);

    // ── NumericUpDown ──
    RESOLVE(CreateNumericUpDown);
    RESOLVE(GetNumericUpDownValue);
    RESOLVE(SetNumericUpDownValue);
    RESOLVE(SetNumericUpDownRange);
    RESOLVE(SetNumericUpDownStep);
    RESOLVE(SetNumericUpDownPageStep);
    RESOLVE(SetNumericUpDownDecimals);
    RESOLVE(SetNumericUpDownReadOnly);
    RESOLVE(SetNumericUpDownPlaceholder);
    RESOLVE(SetNumericUpDownButtonPosition);
    RESOLVE(SetNumericUpDownButtonWidth);
    RESOLVE(SetOnNumericUpDownValueChanged);
#undef RESOLVE
}

static int runTest(const char* shortName, const char* displayName) {
    printf("=== test_numericupdown_cabi: UICornerstone.dll + %s ===\n", displayName);

    g_uiDll = LoadLibraryA("UICornerstone.dll");
    if (!g_uiDll) { printf("FAIL: LoadLibrary\n"); return 1; }
    printf("OK: loaded UICornerstone.dll\n");

    loadAllProcs(g_uiDll);
    if (!uiInit) {
        printf("FAIL: GetProcAddress(Init)\n");
        FreeLibrary(g_uiDll);
        return 1;
    }

    UIBackendCallbacks* callbacks = GetUIBackendCallbacks();
    if (!callbacks) {
        printf("FAIL: GetUIBackendCallbacks\n");
        FreeLibrary(g_uiDll);
        return 1;
    }

    if (!uiInit(callbacks)) {
        printf("FAIL: Init\n");
        FreeLibrary(g_uiDll);
        return 1;
    }
    uiSetViewport(0, 0, 600, 480);
    printf("OK: initialized\n");

    // ── 注册回调 ──
    uiRegisterAction("onValueChanged", (void(*)(void*,void*))onValueChanged, nullptr);

    // ── 用 JSON 加载主面板 + 标题/状态标签 ──
    const char* layoutJson = R"json({
        "version": "1.0",
        "controls": [
            {
                "type": "Panel",
                "id": "rootPanel",
                "rect": { "x": 0, "y": 0, "w": 600, "h": 480 },
                "colors": { "background": { "normal": "#282828FF" } },
                "children": [
                    {
                        "id": "lblTitle",
                        "type": "Label",
                        "rect": { "x": 20, "y": 16, "w": 560, "h": 28 },
                        "caption": "NumericUpDown C ABI Test",
                        "fontSize": 20,
                        "textColor": [220, 220, 220]
                    },
                    {
                        "id": "lblHint1",
                        "type": "Label",
                        "rect": { "x": 20, "y": 56, "w": 200, "h": 22 },
                        "caption": "nudInteger (step=1)",
                        "fontSize": 13,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudInteger",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 80, "w": 200, "h": 26 },
                        "value": 50,
                        "range": { "min": 0, "max": 100 },
                        "step": 1,
                        "decimals": 0,
                        "events": { "onValueChanged": "onValueChanged" }
                    },
                    {
                        "id": "lblHint2",
                        "type": "Label",
                        "rect": { "x": 20, "y": 116, "w": 200, "h": 22 },
                        "caption": "nudFloat02 (step=0.2, decimals=2)",
                        "fontSize": 13,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudFloat02",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 140, "w": 200, "h": 26 },
                        "value": 0.6,
                        "range": { "min": 0.0, "max": 1.0 },
                        "step": 0.2,
                        "decimals": 2,
                        "events": { "onValueChanged": "onValueChanged" }
                    },
                    {
                        "id": "lblHint3",
                        "type": "Label",
                        "rect": { "x": 20, "y": 176, "w": 200, "h": 22 },
                        "caption": "nudBigStep (step=50)",
                        "fontSize": 13,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudBigStep",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 200, "w": 200, "h": 26 },
                        "value": 100,
                        "range": { "min": 0, "max": 1000 },
                        "step": 50,
                        "events": { "onValueChanged": "onValueChanged" }
                    },
                    {
                        "id": "lblHint4",
                        "type": "Label",
                        "rect": { "x": 20, "y": 236, "w": 200, "h": 22 },
                        "caption": "nudReadOnly",
                        "fontSize": 13,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudReadOnly",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 260, "w": 200, "h": 26 },
                        "value": 42,
                        "range": { "min": 0, "max": 100 },
                        "readOnly": true,
                        "events": { "onValueChanged": "onValueChanged" }
                    },
                    {
                        "id": "lblHint5",
                        "type": "Label",
                        "rect": { "x": 20, "y": 296, "w": 200, "h": 22 },
                        "caption": "nudInline (InlineEditBox)",
                        "fontSize": 13,
                        "textColor": [180, 180, 180]
                    },
                    {
                        "id": "nudInline",
                        "type": "NumericUpDown",
                        "rect": { "x": 20, "y": 320, "w": 200, "h": 26 },
                        "value": 3.14,
                        "range": { "min": 0.0, "max": 10.0 },
                        "step": 0.01,
                        "decimals": 2,
                        "events": { "onValueChanged": "onValueChanged" }
                    },
                    {
                        "id": "lblStatus",
                        "type": "Label",
                        "rect": { "x": 20, "y": 380, "w": 560, "h": 24 },
                        "caption": "Last: (none)",
                        "fontSize": 14,
                        "textColor": [180, 200, 220]
                    },
                    {
                        "id": "btnQuit",
                        "type": "Button",
                        "rect": { "x": 480, "y": 420, "w": 100, "h": 36 },
                        "caption": "Quit",
                        "events": { "onClick": "onQuitClicked" }
                    }
                ]
            }
        ]
    })json";

    if (!uiLoadLayout(layoutJson)) {
        printf("FAIL: LoadLayout\n");
        uiShutdown();
        FreeLibrary(g_uiDll);
        return 1;
    }
    printf("OK: layout loaded (5 NumericUpDown + 5 Label + 1 Button)\n");

    // ── 演示：通过 C ABI 修改运行时属性 ──
    void* nudFloat02 = uiFindControl("nudFloat02");
    if (nudFloat02 && uiSetNumericUpDownValue) {
        uiSetNumericUpDownValue(nudFloat02, 0.8);
        printf("OK: nudFloat02 = %.2f\n", uiGetNumericUpDownValue(nudFloat02));
    }

    void* nudInline = uiFindControl("nudInline");
    if (nudInline && uiSetNumericUpDownButtonPosition) {
        // 运行时切换为内嵌模式（验证 buttonPosition API）
        uiSetNumericUpDownButtonPosition(nudInline, 1);  // 1 = InlineEditBox
        printf("OK: nudInline buttonPosition set to InlineEditBox\n");
    }

    printf("Frame loop... (interact with NumericUpDowns or click Quit)\n");
    while (!uiIsQuitRequested()) {
        uiProcessEvents();
        uiUpdate(1.0 / 60.0);
        uiClear();
        uiRender();
        uiPresent();
    }

    uiShutdown();
    FreeLibrary(g_uiDll);
    g_uiDll = nullptr;
    printf("test_numericupdown_cabi_%s: done\n", shortName);
    return 0;
}

int main() { return runTest(BACKEND_SHORT_NAME, BACKEND_DISPLAY_NAME); }
```

#### 12.4.3 关键点（与 test_combobox_cabi 的差异）

| 维度         | test_combobox_cabi.cpp                | test_numericupdown_cabi.cpp                |
|--------------|---------------------------------------|---------------------------------------------|
| 控件类型     | 1 ComboBox（带 10 个 items）          | 5 NumericUpDown（不同 step/decimals/模式） |
| JSON 关键字段| `items: [{label, value, disabled?}]`  | `value/range/step/decimals/readOnly/buttonPosition` |
| 回调签名     | `void(*)(void*, void*)`               | `void(*)(void*, double)`                    |
| C ABI 函数   | SetComboItems/GetSelectedIndex/Label  | SetValue/GetValue/SetRange/SetStep/SetDecimals/SetReadOnly/SetButtonPosition |
| 演示重点     | 单击下拉、禁用项、selected index      | 多 step 对比、InlineEditBox 模式切换        |

## 13. 优化历史

（待实现后填写）

### 2026-XX-XX: 初始实现

- 完成上述全部 14 个阶段
- 三后端（SDL3/SFML/Raylib）DLL 模式全部编译通过
- test_numericupdown + test_fromsource_cabi 三后端 NumericUpDown 控件可见可交互
- JSON LayoutParser 全参数/缺省参数解析正确
- C ABI 12 个函数（含 pageStep）全部从 test_fromsource_cabi 验证






