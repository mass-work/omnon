# omnon

omnon はトラックボールとジョイスティックを組み合わせた制作キットです。  
可愛さと遊び心を取り入れたシンプルな自作入力デバイスです。

<img src="doc/images/001_01_top.jpg" width="800px">

## 特徴

- **多様な入力に対応**：ジョイスティックやトラックボール(OP)を操作することで、色々な操作を行うことが出来ます。
- **ジョイスティックの Vial 対応**：ボタンやジョイスティックのカスタマイズをプログラムを書かずに GUI で行えます。

---

- [omnon](#omnon)
  - [特徴](#特徴)
  - [機能の説明と設定](#機能の説明と設定)
    - [Vial](#vial)
    - [ジョイスティックの使い方と設定方法](#ジョイスティックの使い方と設定方法)
    - [各レイヤーの説明](#各レイヤーの説明)
    - [動作例](#動作例)
    - [トラックボールの使い方と設定方法](#トラックボールの使い方と設定方法)
    - [ボタンの設定](#ボタンの設定)
  - [注意点・免責](#注意点免責)

---

## 機能の説明と設定

### Vial

各種設定は[Vial](https://vial.rocks/)により行うことが出来ます。

1. リンクにアクセス後 Start Vial を押下してください。

   <img src="doc/images/100_01_readme_vialtop.jpg" width="300px">

2. omnon への接続を要求されるので接続してください。

   <img src="doc/images/100_01_readme_vialconn.jpg" width="300px">

3. 画面上半分は、omnon の設定状態になります。  
   omnon の変更したい箇所のボタンをクリック後、下半分にある任意のキーを選択することでキーが変更されます。  
   ジョイスティックやトラックボールへの設定を変更するには、USB を再接続(抜き差し)する事で設定が反映されます。

   <img src="doc/images/100_01_readme_vialomnon.jpg" width="500px">

> [!CAUTION]
> 通常 Vial ではキー変更のみで反映されます。  
> omnon はデバイスの特性上仮想キーを使用しジョイスティックやトラックボールへ割り当てを行っています。  
> 変更後は USB を一度差し直すことで、仮想キーの設定がジョイスティックやトラックボールへ反映されます。

### ジョイスティックの使い方と設定方法

---

- ジョイスティックを倒すことでレイヤー 0 に設定されたキーが繰り返し入力されます。
- 倒した角度により、繰り返し入力される頻度が変化します。
- レイヤー 1 ～ 3 の順に設定されたキーが押下(Hold or Tap)された後、レイヤー 0 のキーが Tap されます。

### 各レイヤーの説明

**レイヤー 0**：ジョイスティック操作時に繰り返し入力されるキーを設定します。

 <img src="doc/images/100_01_readme_layer0.jpg" width="350px">

**レイヤー 1 ～レイヤー 3**：レイヤー 0 の操作前にホールドまたはタップするキーを設定します。

 <img src="doc/images/100_01_readme_layer13.jpg" width="1200px">

### 動作例

例：レイヤーの説明画像の場合、下記のように動作します。

- **joy stick(L) 上下 ：** Vol + or Vol -
- **joy stick(L) 左右 ：** LCtrl(Hold) → Lgui(Hold) → Left or Right → Lgui(unHold) → LCtrl(unHold)  
  ※仮想デスクトップ切替え
- **joy stick(R) 上下 ：** Esc(Tap) → Lgui(Hold) → UP or Down → Lgui(unHold)  
  ※window 移動
- **joy stick(R) 左右 ：** Esc(Tap) → Lgui(Hold) → Left or Right → Lgui(unHold)  
  ※window 移動

これにより、簡単にジョイスティック操作を変更し、様々な操作ができます。

---

### トラックボールの使い方と設定方法

トラックボールはポインタ操作やスクロールを行うことが出来ます。

**レイヤー 0**に設定することで、カスタマイズをすることが出来ます

> [!CAUTION]
> LR それぞれマウスセンサーを付けていない所は必ず blank(空白)を設定してください。  
> blank(空白)を設定していない場合、センサーがついている側のマウスセンサーも動かない場合があります。

 <img src="doc/images/100_01_readme_tb1.jpg" width="350px">

例：説明画像の場合、下記のようになります。  
**左トラックボール ：** 上下左右のスクロール操作  
**右トラックボール ：** カーソル操作  
**cpi(感度) ：** 左右ともに 3000

また、左右ともにマウスセンサーがついている場合、同時に操作することで矢印キーの入力になります。  
**同時操作 ：** 上下左右の矢印キー操作  
※この機能はベータ版です、将来的には削除される可能性があります。  
　また、現状では左右のどちらかにスクロールを割り当てることが必須です。

---

### ボタンの設定

赤丸部はレイヤー 0 で任意のボタンを設定することが出来ます。

<img src="doc/images/100_01_readme_button.jpg" width="350px">

---

## 注意点・免責

omnon は現在開発中バージョンです。  
仕様は、予告なく変更される可能性があります。  
変更が生じた場合、最新の情報は omnon の github をご確認ください。
