# AviUtl プラグイン - フィルタのコピペ

* 1.0.0 by 蛇色 - 2022/04/28 初版

設定ダイアログのフィルタをコピペできるようにします。

## 導入方法

1. 以下のファイルを AviUtl の Plugins フォルダに配置します。
	* CopyFilter.auf

## 使用方法

1. 設定ダイアログで右クリックしてコンテキストメニューを出します。
2. メニュー項目が追加されているはずなので必要なものを選択します。

## 制限事項

### オブジェクトの分類

* 映像メディアオブジェクト(グループ制御含む)
* 映像フィルタオブジェクト
* 音声メディアオブジェクト
* 音声フィルタオブジェクト
* カメラ制御

オブジェクトはこれらに分類され、同じ分類のオブジェクト同士でしかコピペはできません。

### 先頭のフィルタ

先頭のフィルタはカットやコピーはできません。

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r21 https://scrapbox.io/ePi5131/patch.aul

## クレジット

* Microsoft Research Detours Package https://github.com/microsoft/Detours
* aviutl_exedit_sdk https://github.com/ePi5131/aviutl_exedit_sdk
* Common Library https://github.com/hebiiro/Common-Library
