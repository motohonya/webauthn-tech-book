= WebAuthn

== 概要

//image[webauthn_overview][FIDO2 Overview]

@<img>{webauthn_overview} は Web Authentication API（以下WebAuthn） と Platform, および、External Authenticator の関係図です。
FIDO の文脈における RP（Relying Party）とは Authenticator に認証を委任するサーバーを指し、一般には Id Provider などの ID 提供者であることが多いです。

前章で解説したとおり、FIDO2.0 は WebAuthn と、 CTAP1/CTAP2 および Platform API を、
それぞれの OS,Browser ベンダーが実装して実現するプロジェクトです。
この図を見れば解るとおり、WebAuthn の裏側には、 Platform API や CTAP が存在することがわかります。

Platform API は、Platform、つまり PC や スマートフォンに登載された TPM や TEE といったセキュアチップと通信を行います。
PIN あるいはデバイスに登載された生体認証デバイスによって、セキュアチップ上での秘密鍵の生成や保存のための認証が行われます。
つまり、スマートフォンに登載された指紋デバイスなどで、Webの認証が可能になります。
Internal Authenticator を利用する API は、ネイティブアプリケーション、あるいは Web ブラウザーから呼び出せるようになっています。
Android や、Windows 10 で Internal Authenticator が利用できるかどうかは isUserVerifyingPlatformAuthenticatorAvailable() というメソッドで判定ができるようになっているようです。

一方 CTAP は YubiKey などの外部 Authenticator との通信仕様を定めており、これにより USB等で接続された外部デバイスで Web 認証が可能になります。

== CTAP

CTAP とは Client to Authenticator Protocol@<fn>{CTAP} の略で、Client が roaming Authenticator と通信するための仕様です。
roaming Authenticator とは、持ち運びが可能な外付けの認証デバイスであり、 CTAP では Client と USB/NFC/Bluetooth のいずれかで通信するように定められています。
YubiKey や Google の Titan Key などが、有名です。

CTAP には CTAP1 と CTAP2 が存在し、CTAP1 は U2F プロトコルを指します。
一方 CTAP2 は CTAP1 を拡張した規格です。CTAP2 では CTAP1 では実現できなかったパスワードレス認証や、PIN や 生体認証を利用したマルチファクター認証がスペックとして追加されています。

//footnote[CTAP][Client To Authenticator Protocol: https://fidoalliance.org/specs/fido-v2.0-id-20180227/fido-client-to-authenticator-protocol-v2.0-id-20180227.html]

====[column] U2F と CTAP

WebAuthn では U2Fプロトコルに対応した Authenticator を利用することが可能です。
少し説明が難しいのですが WebAuthn と従来の U2Fプロトコルとは一部パラメータ等が異なります。
具体的には U2F では AppID、WebAuthn では RP ID と呼ばれているパラメータが異なっており、前者がプロトコル名 (https://) を含むのに対し、後者は ドメイン名しか含みません。
そのため、現在 U2F で認証をしているサービスをそのまま、WebAuthn に切り替えることはできません。
ただし、WebAuthn のスペックには、この差を埋めるための Extension（AppIdExtention） が定義されており、
U2Fの認証サービスから WebAuthn への切り替えをスムーズにするための工夫がされています。

== Web Authentication API

Web Authentication API は W3C で策定が進められている API で、ウェブブラウザ上に実装されています。
このAPIですが、実は Credential Management API @<fn>{CDM} という Credential を管理する API を拡張する形で実装されています。

Credential Management API にはたった2つのメソッドが定義されています。
資格情報を作成する credentials.create() および、取得する credentials.get() です。
WebAuthn ではこの2つのメソッドに、公開鍵を利用した資格情報を作成するためのオプションである、
PublicKeyCredentialCreationOptions や PublicKeyCredentialRequestOptions を指定して、Credential を作成します。

//listnum[PublicKeyCredential][2つのメソッドと、2つのオプション][js]{

navigator.credentials.create({publicKey: PublicKeyCredentialCreationOptions})

navigator.credentials.get({publicKey: PublicKeyCredentialRequestOptions})

//}


通常これらのオプションは、サービス提供者のポリシーや、ユーザーの選択によってパラメーターを変更することになります。
ただしオプションに関しては数も非常に多く、複雑であるため、本書では基本的な流れをとらえてるためシンプルなオプションで解説をすすめたいと思います。

//footnote[CDM][Credential Management API : @<href>{https://www.w3.org/TR/credential-management-1/}]

=== Credential の作成

@<img>{WebAuthn_Registration_r4} は MDN Web docs に記載されている WebAuthn の登録フローです。

//image[WebAuthn_Registration_r4][Registration* https://developer.mozilla.org/ja/docs/Web/API/Web_Authentication_API]

Credential Management API のメソッドである navigator.credentials.create() の引数である、CredentialCreationOptions 
として、{ publicKey: options } といった形で WebAuthn で利用するオプションを渡しています。
これらのオプションは  PublicKeyCredentialCreationOptions とよばれ、
「誰に対して」「どのような Authenticator」を利用して「どのRPの」 Credential を作成するか、といったことを定めます。

もっともシンプルな例として @<list>{create} を見てみましょう。

@<list>{create} は必須のオプションのみ指定した credentials.create() メソッドの例です。
@<strong>{challenge} は rpサーバーから送られるランダムなバイト列です。この例では、説明のため js 上で生成していますが、
他のパラメータ含め、通常サーバー側から送られることに注意してください。

//pagebreak

//listnum[create][navigator.credentails.create()][js]{

navigator.credentials.create({
    publicKey: {
        challenge: new Uint8Array(
          [21,31,105 /* 29 more random bytes generated by the server */]
        ),

        rp: {
            id: "example.com" //省略可能
            name: "ACME Corporation"
        },

        user: {
            id: Uint8Array.from(
              window.atob("MIIBkzCCATigAwIBAjCCAZMwggE4oAMCAQIwggGTMII="), 
              c=>c.charCodeAt(0)
            ),
			displayName: "watahani",
        },

        pubKeyCredParams: [
            {
                type: "public-key",
                alg: -7 // "ES256" IANA COSE Algorithms registry
            },
            {
                type: "public-key",
                alg: -257 // Value registered by this specification for "RS256"
            }
        ]
    }
})

//}


@<strong>{rp} は認証先のRPに関するパラメーターで、@<strong>{name} は必須のパラメーターです。
@<strong>{id} は省略可能なパラメーターで、省略した場合は現在アクセスしている origin のドメイン名と同一の値になります。
また、@<strong>{rp.id} を直接指定することも可能です。
たとえば subdomain.example.com 上で、rp.id を指定しない場合、暗黙的に rp.id は subdomain.example.com になります。
しかし、subdomain.example.com 上では、@<strong>{rp.id} としてexample.com を指定することも可能です。
そうしたばあい、 *.example.com で使いまわすことが可能な Credential を作成することが可能です。

@<strong>{user} は Credential を利用するユーザーに関するパラメーターです。
@<strong>{displayName} は、@<strong>{rp.name} と合わせて、ユーザーへの同意画面で利用されたり、
Authenticator 上で、アカウント情報を表示する際などに利用されます。
@<strong>{user.id} はサービス固有の ID で利用者に対して表示することはありませんが必ずユニークな id である必要があります。
一般的にはサービスの内部で利用しているユーザーのユニークidを利用することになるでしょう。

@<strong>{pubKeyCredParams} は、 Authenticator が使用する署名アルゴリズムを指しており、
WebAuthn では "-7": ES256 (ECDSA w/ SHA-256) および、"-257": RS256 (RSASSA-PKCS1-v1_5 w/ SHA-256) の
2つの署名アルゴリズムのいずれか、あるいは両方を指定できます。
現在利用できる Authenticator でいうと Windows Hello で利用する TPM が RS256 を、@<fn>{tpm}
それ以外の Authenticator が ES256 を利用しています。

//footnote[tpm][Windows Hello 以外で RS256 を利用する Authenticator は出てこないんじゃないかなと思われる]

=== Credential Create の戻り値

navigator.credentials.create() メソッドで公開鍵暗号ベースの Credential を作成しようとすると、Authenticator へこれらの情報が送られます。
Authenticator は Credential を作成するためにユーザー認証を行います。@<fn>{up-uv}

//footnote[up-uv][User Presence および User Verification（詳しくはコラム参照）]

ユーザー認証が完了すると Authenticator は作成した Credentail を Client に返します。
レスポンスは、PublicKeyCredential として navigator.credentials.create() の返す Promise オブジェクトの戻り値として返されます。
PublicKeyCredential は@<list>{PublicKeyCredential} のような形となります。

//listnum[PublicKeyCredential][PublicKeyCredential]{
    rawId: [ArrayBuffer] (32 bytes)
        3C B4 0D 54 85 0C 2A 39 EF 4F 9B A5 E7 5C 66 72
        C1 CB 8D 02 54 66 0A 0B 88 07 AE 09 4A 55 08 6D,
    id: [ArrayBuffer] (32 bytes)
        3C B4 0D 54 85 0C 2A 39 EF 4F 9B A5 E7 5C 66 72
        C1 CB 8D 02 54 66 0A 0B 88 07 AE 09 4A 55 08 6D,
    response: {
        clientDataJSON: [ArrayBuffer],
        attestationObject: [ArrayBuffer],
    },
    getClientExtensionResults: () => {
      return authenticationExtensionsClientOutputs;
    }
}
//}

id は、@<strong>{CredentialId} と呼ばれ生成された PublicKeyCredential を一意に定めるIDです。
@<strong>{response} は @<strong>{AuthenticatorAttestationResponse} と呼ばれ、 clientDataJSON および AttestationObject を含むオブジェクトです。 
@<strong>{clientDataJSON} は、Client（ブラウザー）で生成された @<strong>{challenge} を含むデータで、実際には JSON を Base64Url エンコードしたバイト配列です。
origin の検証、Attestationの検証に利用されます。

//listnum[clientDataJSON][clientDataJSON type は webauthn.create もしくは webauthn.get]{
{
    "type" : "webauthn.create",
    "challenge" : "OVtTIlKnwUaR-MsMYiD6D8g0zqntXIe-g2hs...",
    "origin" : "https://example.com",
    "tokenBinding" : {"status" : "supported"}
}
//}

@<strong>{AttestationObject} は、いくつかの重要な情報を含んでいますが、
ここでは Authenticator で生成された公開鍵と、Reply攻撃を防ぐため、署名をするたびに増える Counter を含む、とだけにとどめておきます。
サーバーへのレスポンスは CredentailId, clientDataJSON, attestationObject（に含まれる公開鍵およびCounter）が送られ、登録ユーザーに紐づけて保存されます。
詳しいサーバー側での処理は Spec 7.1. Registering a New Credential@<fn>{registering-a-new-credential} を参照ください。

//footnote[registering-a-new-credential][https://w3c.github.io/webauthn/#registering-a-new-credential]

====[column] User Presence と User Verification

WebAuthn ではユーザーの認証ジェスチャーとして、User Presence（UP） と User Verification（UV） の2つを定めています。
UP はユーザーが Authenticator にタッチするなどの簡単な動作を指します。
YubiKey であれば、金属リング部分をタッチするといった動作になります。
これは Authenticator の近くにユーザーが存在し、 Credential の作成に同意したことを簡単な動作で示すことを意味します。
UP は WebAuthn のプロトコルでは必須であるため、Authenticator は何らかの形で User Presence を確認する必要があります。

一方 User Verification（UV） は、Authenticator が各ユーザーを識別するために、
生体認証や PIN を用いてユーザー検証を行うことを指します。
スマートフォンの生体認証や PIN の入力を行うことで、 Credential の利用者本人であることを確認します。
UV は WebAuthn では、optional ですが、Credential の作成、使用の際のオプションとして
必ず UV を使用するといった指定をすることが可能です。

User Presence と User Verification は、通常同時に行われることが多いですが、
YubiKey など、生体認証を利用できない Authenticator などでは、キーへのタッチ・PINの入力を
別々に行わなければいけないことに注意してください。

ちなみに Android や Microsoft Hello で利用する PIN は、User Verification も兼ねているようです。
おそらく、PIN の入力をローカルリソースからでなければできないような仕組みにしているのだと思いますが詳細は不明です。

=== 作成した Credential を利用した認証（Assertion）

作成された PublicKeyCredential はサーバーに送られ次回以降の認証で利用されます。
一方 Authenticator も CredentialId と PublicKeyCredential の組み合わせを記憶しており、次回の認証時に CredentailId を直接指定することも可能で、
サーバーに送った公開鍵に対応する秘密鍵を取り出すことが可能になっています。

認証の際は navigator.credentials.get() に {publicKey: PublicKeyCredentialRequestOptions} といった形のオプションを指定し、既存の PublicKeyCredential を取得します。
@<list>{get} は @<list>{create} で生成した、 3C B4 0D 54... という CredentailId で指定する PublicKeyCredential を利用して認証をするというリクエストです。

//listnum[get][navigator.credentials.get メソッド]{

navigator.credentials.get({
    publicKey: {
        challenge: new Uint8Array(
          [21,31,105 /* 29 more random bytes generated by the server */]
        ),
        allowCredentials: [
        {
            id: [ArrayBuffer] (32 bytes)
                3C B4 0D 54 85 0C 2A 39 EF 4F 9B A5 E7 5C 66 72
                C1 CB 8D 02 54 66 0A 0B 88 07 AE 09 4A 55 08 6D,
            type: "public-key",
        },
    ],
    }
})

//}

@<strong>{challenge} は先ほどと同様にサーバーから送られるランダムなバイト配列です。
今回は id(3C B4 0D...) に対応する PublicKeyCredential で、Challenge に署名をして返します。

このメソッドが呼ばれると Client は接続している Authenticator を列挙し、サーバーから送られた CredentailId に対応する Authenticator を探します。
ユーザーの認証処理が完了すると、Authenticator は CredentialId に対応する PublicKeyCredential を返します。

=== Assertion の検証

ユーザーの認証処理が完了すると Authenticator は Client にレスポンスを返します。
レスポンスは、PublicKeyCredential として navigator.credentials.get() の返す Promise オブジェクトの戻り値として返されます。
 Credential を作成した際のレスポンスである @<list>{PublicKeyCredential} とは異なり、
response には AuthenticatorAssertionResponse がセットされ、publicKey などが含まれる AttestationObject は含まれておらず、
代わりに authenticatorData と signature が返されます。

ClientData は credentials.create() の際の @<list>{clientDataJSON}同様で、type のみ webauthn.get となっています。
authenticatorData は、さまざまなデータを含みますが、今は RP ID Hash, 1yte の　Flags（UP,UV の結果を含む）, Counter が含まれるものだと思ってください。

@<strong>{signature} は credentials.get() で作成した publicKey に対応する秘密鍵で作成した署名で、
authenticatordata と clientDatan の SHA256 ハッシュを計算した clientDataHash を結合したバイト配列に対して行われます。

//listnum[AuthenticatorAssertionResponse][AuthenticatorAssertionResponse]{
{
    rawId: [ArrayBuffer] (32 bytes)
        3C B4 0D 54 85 0C 2A 39 EF 4F 9B A5 E7 5C 66 72
        C1 CB 8D 02 54 66 0A 0B 88 07 AE 09 4A 55 08 6D,
    id: [ArrayBuffer] (32 bytes)
        3C B4 0D 54 85 0C 2A 39 EF 4F 9B A5 E7 5C 66 72
        C1 CB 8D 02 54 66 0A 0B 88 07 AE 09 4A 55 08 6D,
    response: {
        clientDataJSON: [ArrayBuffer],
        authenticatorData: [ArrayBuffer],
        signature: [ArrayBuffer],
        userhandle: [ArrayBuffer]
    },
    getClientExtensionResults: () => {
      return authenticationExtensionsClientOutputs;
    }
}
//}

サーバー側では ClientDataJSON に含まれる challenge, origin の検証、authenticatorData に含まれる rpId の検証、Flags の検証を行った後
保存されている publicKey で署名の検証を行います。
最後に、Counter が前回認証時よりも増えているかの検証を行い、認証を完了します。

詳しいサーバー側での処理は Spec 7.2. Verifying an Authentication Assertion@<fn>{verifying-assertion} を参照ください。

//footnote[verifying-assertion][https://w3c.github.io/webauthn/#verifying-assertion]

=== WebAuthn のポイント

ここまでが Webauthn の Credential の作成および、保存された Credential を用いた認証の基本的な流れです。
WebAuthn のポイントとしては次の4つです。

    1. Web Authentication API は 公開鍵暗号を利用したチャレンジレスポンス認証
    1. Web Authentication API は Credential Management API の拡張として定義
    1. API は credentials.create() と credentials.get() のふたつの認証API
    1. CredentialId を利用して、認証に用いる Credentail を指定する

WebAuthn には、UserVerification を求めるオプションや、
Authenticator 内に Credential を保存してユーザー名を入れずにパスワードレス認証を行う仕様もあります。


=== ResidentKey について

ResidentKey とは、Credential を Authenticator に保存して、ユーザー情報の入力なしに認証するためのオプションです。
イメージとしては UAF の認証に近く、スマートフォンでの指紋認証や、YubiKey によるタッチだけで認証が完了します。
ResidentKey を利用するには FIDO2 対応の YubiKey 5 や Security Key by Yubico といったデバイスと RS5 以上の Windows 10 および Edge が必要@<fn>{win10}です。
今回は日本国内でも発売されている Security Key by Yubico を利用した ResidentKey の動作を確認します。

//footnote[win10][Windows Hello でも ResidentKey オプションは動作します]

==== ResidentKey を利用した Credential の作成

ユーザー情報を入力することなく認証を行うために ResidentKey では Credential をデバイス内に保存します。
@<list>{authenticatorselection} は、 publicKeyCredentialCreationOptions の authenticatorSelection オプションで、
利用する Authenticator を制限します。この例では Security Key by Yubico といった ResidentKey オプションに対応した外部 Authenticator を指定しています。

//listnum[authenticatorselection][authenticatorselection]{

authenticatorSelection: {
     authenticatorAttachment: “cross-platform”,
     userVerification: “required“,
     requireResidentKey: true
}

//}

このようなオプションを指定すると Client は、次のような動きをします。

    1. CTAP を利用した外部 Authenticator に対し、
    1. User Verification を必須にしたうえで
    1. ユーザー情報を保存する

YubiKey は第3章ですこし解説していますが、credentialId からキーペアの導出が可能です。
そこで、FIDO2対応の ResidentKey では credentialId と user の情報を rpId に紐づけて保存します。
今回は User Verification も必須としているので、Credential を作成する前に Edge が YubiKey の PIN を要求します。
PIN の入力と YubiKey へのタッチが完了すると Credential が YubiKey に保存され、 publicKey は従来どおり RP に送られます。

//image[regidentKey_make][ResidentKey を利用した Credentail の生成]


==== ResidentKey を利用した Credential の利用

一方認証の際には、今まで RP から allowCredentials として送信していた CredentialId は不要になります。
ゆえに allowCredentials は空にし、rpId のみ指定して credentials.get() メソッドを呼び出します。

//listnum[get-ResidentKey][regidentKey を利用した認証]{

navigator.credentails.get({
    "publicKey": {
        challenge: challenge
        allowCredentials: [],
        rpId: rpId
        userVerification = "required",
    }
})

//}

このように allowCredentials を空にすることによって、ResidentKey を利用することとなります。
このようなオプションを指定すると Client は接続された Authenticator に対し、rpId と ClientDataHash のみを渡して保存された Credential のリストを要求します。
User Verification と User Presence が確認できたのち、Authenticator は保存された Credential のリスト（実際には signature も含む）を Client に返します。
Client はユーザーに対し、Authenticator が返した Credential のリストを提示しユーザーが選択した Credentail情報（signature 含む）を RP に送ります。
その際、 UserHandle と呼ばれるパラメータ（実際には登録の際の user.id）も RP に送られ、 RP でユーザーを特定するのに利用されます。

//image[residentKey_auth][ResidentKey を利用したユーザネームレス認証]

ここでは　User Verification の説明のために PIN の入力フローも合わせて説明しましたが、実際には User Verification なしでの ResidentKey の利用も可能です。
キーボードがなくユーザー名や PIN の入力が難しかったり、手袋を利用していて YubiKey のタップが難しい現場などでは、NFCを利用してワンタップログインといった運用が可能です。

=== Attestation について

ここまでの解説で、説明を省いた Attestation について説明します。
Attestation は Credential の生成時に、Authenticator から返され、
主に は Authenticator で生成された publicKey が、「どのような Authenticator 上で生成されたものか」を RP が確認するために用いられます。
これは主にエンタープライズ向けに信頼された Authenticator のみを利用するように制限するといった、ユースケースで用いられます。
デフォルトのオプションでは Attestation は Client で削除され、RPには送られません。そのため、Attestation が必要な場合は、明示的なオプションを指定してやる必要があります。

//pagebreak

//listnum[attestation][AttestationConveyancePreference]{
let publicKeyCredentialCreationOptions = {
    challenge: challenge,
    rp: { name: "ACME Corporation"},
    user: {
        id: userId
        displayName: "watahani",
    },

    pubKeyCredParams: [...]

    attestation = "direct";
}
//}

さて、Attestation にはいくつか種類があるのですが、いずれも Authenticator 内で、ClientData と rpId などに Attestation Private Key で署名を行い、
サーバー側で Private Key に対応する Public Key を含む証明書などで署名の検証、証明書チェインの検証を行います。
たとえば、もっともシンプルな U2F Attestation では、YubiKey内部に埋め込まれた Attestaion Private Key で rpId, clientDataHash, credentialId, publicKey に対して署名（Attestation signature）を生成します。
同時に Yubico の Root CA に署名された Attestation Certificate とともに AttestationObject としてサーバーに返すことによって、署名の検証および、証明書の検証をすることができます。
Attestationの検証を行うことによって、Authenticator から返された CredentialId と publicKey が、正規の YubiKey で生成されたことが確認できます。

//image[u2f_attestation][U2F attestation]

なお、Attestation の情報に関しては FIDO Metadata Service という、Authenticator の Attestation 等を登録するサービスがあり、各 Authenticator の情報が取得できるようになる予定ですが
現在のところ、登録された Authenticator はほとんどなく、あまり活用されているようではありません。
