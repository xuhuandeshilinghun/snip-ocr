/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#include "sticker_window.h"
#include "i18n.h"
#include "ui_theme.h"
#include "config_manager.h"
#include "resource_helper.h"
#include "Snip_ocr_app.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QEnterEvent>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QToolButton>
#include <QButtonGroup>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QTimer>
#include <QFontMetrics>
#include <QLineEdit>
#include <QColorDialog>
#include <QPushButton>
#include <QGridLayout>
#include <QSlider>
#include <QSpinBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QComboBox>
#include <QSvgRenderer>
#include <QRegularExpression>
#include <QByteArray>
#include <QIcon>
#include <QPixmap>
#include <QImage>
#include <QPolygonF>
#include <QPointF>
#include <QShowEvent>
#include <QAction>
#include <QHash>
#include <QDataStream>
#include <QBuffer>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QRandomGenerator>
#include <cmath>
#include <QTransform>
#include <QProcess>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QSet>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QInputDialog>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace sticker {
bool StickerWindow::s_topmostSuspended = false;


static const QString MOSAIC_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M512 64h448v896H64V64h448z m192 64h-192v384h384V128h-192zM320 512H128v384h384V512h-192z\"/>"
    "</svg>");

static const QString RECT_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M841.34 959.36H182.66c-65.06 0-117.99-52.94-117.99-118.02V182.69c0-65.08 52.94-118.04 117.99-118.04h658.68c65.06 0 117.99 52.96 117.99 118.04v658.65c0 65.08-52.93 118.02-117.99 118.02zM182.66 142.17c-22.31 0-40.51 18.18-40.51 40.51v658.65c0 22.34 18.2 40.49 40.51 40.49h658.68c22.31 0 40.51-18.15 40.51-40.49V182.69c0-22.34-18.2-40.51-40.51-40.51H182.66z\" fill=\"#2c2c2c\"/>"
    "</svg>");

static const QString ELLIPSE_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M512 802c-115.774 0-225.359-27.495-308.567-77.42-42.745-25.646-76.681-56.046-100.865-90.353C75.649 596.042 62 554.919 62 512c0-42.918 13.649-84.042 40.567-122.228 24.185-34.307 58.121-64.706 100.865-90.353C286.641 249.495 396.226 222 512 222s225.359 27.495 308.567 77.42c42.745 25.646 76.681 56.046 100.865 90.353C948.352 427.958 962 469.082 962 512c0 42.919-13.648 84.042-40.567 122.228-24.185 34.307-58.12 64.706-100.865 90.353C737.359 774.505 627.774 802 512 802z m0-480c-97.913 0-189.226 22.434-257.118 63.169C220.194 405.982 162 450.136 162 512c0 61.865 58.194 106.018 92.882 126.831C322.774 679.566 414.087 702 512 702s189.227-22.434 257.118-63.169C803.807 618.018 862 573.865 862 512c0-61.865-58.193-106.018-92.882-126.831C701.227 344.434 609.913 322 512 322z\" fill=\"#2c2c2c\"/>"
    "</svg>");

static const QString RECT_HOLLOW_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M896 832H128V192h768v640zM192 768h640V256H192v512z\"/>"
    "</svg>");
static const QString RECT_FILL_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M896 832H128V192h768v640z\"/>"
    "</svg>");
static const QString ELLIPSE_HOLLOW_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M512 832c-247.4 0-448-143.9-448-320S264.6 192 512 192s448 143.9 448 320-200.6 320-448 320z m0-576c-200.3 0-384 108.9-384 256s183.7 256 384 256 384-108.9 384-256-183.7-256-384-256z\"/>"
    "</svg>");
static const QString ELLIPSE_FILL_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M512 832c-247.4 0-448-143.9-448-320S264.6 192 512 192s448 143.9 448 320-200.6 320-448 320z\"/>"
    "</svg>");

static const QString SHAPE_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M839.058955 1023.999002h-79.584153c-100.954794 0-183.087372-82.141562-183.087372-183.108335V605.075936c0-12.430688 9.467007-22.649346 21.585256-23.841207a23.847196 23.847196 0 0 1 11.642103-3.014589h229.443168c100.964776 0 183.106338 82.132578 183.106338 183.088371v79.582156c0.000998 100.966773-82.139566 183.108335-183.10534 183.108335zM624.302426 626.135136v214.756529c0 74.546195 60.638159 135.194337 135.173374 135.194337h79.584153c74.545197 0 135.19234-60.648141 135.19234-135.194337v-79.582156c0-74.535215-60.647143-135.174373-135.19234-135.174373H624.302426zM262.689493 1023.999002h-79.584153C82.140564 1023.999002 0 941.857439 0 840.890667v-79.582156C0 660.352718 82.140564 578.22014 183.10534 578.22014H412.549506c4.224418 0 8.192295 1.093038 11.63811 3.012593 12.128231 1.18088 21.607217 11.405528 21.607217 23.843203v235.814731c0 100.966773-82.140564 183.108335-183.10534 183.108335z m-79.584153-397.863866C108.560143 626.135136 47.913998 686.774294 47.913998 761.309509v79.582156c0 74.546195 60.646145 135.194337 135.191342 135.194337h79.584153c74.545197 0 135.191342-60.648141 135.191342-135.194337V626.135136H183.10534zM412.550504 445.776865H183.10534C82.140564 445.776865 0 363.643289 0 262.688495v-79.602121C0 82.132578 82.140564 0 183.10534 0h79.584153c100.954794 0 183.088371 82.132578 183.088371 183.087372v235.834696c0 12.430688-9.467007 22.650344-21.585257 23.841207a23.857178 23.857178 0 0 1-11.642103 3.01359zM839.058955 445.776865H609.615787a23.851189 23.851189 0 0 1-11.63811-3.011594c-12.127233-1.18088-21.607217-11.405528-21.607217-23.843203V183.087372c0-100.954794 82.140564-183.087372 183.10534-183.087372h79.584153C940.024729 0 1022.165293 82.132578 1022.165293 183.087372v79.602121c0 100.953796-82.140564 183.087372-183.106338 183.087372z m-214.774497-47.913998h214.774497c74.545197 0 135.19234-60.639157 135.19234-135.174372v-79.602121c0-74.534217-60.647143-135.173374-135.19234-135.173374h-79.584153c-74.545197 0-135.191342 60.639157-135.191342 135.173374v214.776493z\"/>"
    "</svg>");

static const QString ARROW_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M64 448h768v128H64z\" fill=\"#2c2c2c\"/>"
    "<path d=\"M704 256l256 256-256 256V256z\" fill=\"#2c2c2c\"/>"
    "</svg>");

static const QString LINE_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M0 490.666667h1024v42.666666H0z\" fill=\"#2c2c2c\"/>"
    "</svg>");
static const QString ARROW_SIMPLE_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M35.77295909 490.25408215l652.37753843 0L688.15049752 550.65940993 35.77295909 550.65940993l1e-8-60.40532778z\" fill=\"#2c2c2c\"/>"
    "<path d=\"M673.81131166 309.4952894L1025.0901777 515.8019253l-351.27886604 211.88249064 0-418.18912654z\" fill=\"#2c2c2c\"/>"
    "</svg>");

static const QString PEN_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M895.855 313.163l-78.107 78.107-185.04-185.041 78.07-78.107c26.508-26.51 53.017 0 53.017 0l132.06 132.022c0 0.001 26.509 26.51 0 53.019zM116.359 907.654l224.337-39.37-185.452-185.452-38.885 224.822z m64.902-249.993l185.066 185.066 423.009-423.009L604.27 234.652l-423.01 423.01z\"/>"
    "</svg>");

static const QString HIGHLIGHTER_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M650.368 192.469333L362.88 480l-30.165333 90.496-44.373334 44.416 120.661334 120.661333 44.373333-44.373333 90.538667-30.208 287.445333-287.445333-180.992-181.034667z m271.530667 150.869334a42.666667 42.666667 0 0 1 0 60.330666l-331.861334 331.861334-90.538666 30.165333-60.330667 60.373333a42.666667 42.666667 0 0 1-60.330667 0l-181.034666-181.034666a42.666667 42.666667 0 0 1 0-60.330667L258.133333 524.373333l30.165334-90.538666 331.904-331.861334a42.666667 42.666667 0 0 1 60.330666 0l241.365334 241.365334z m-271.530667-30.165334l60.330667 60.330667-211.2 211.2-60.330667-60.330667 211.2-211.2z m-467.626667 407.296l120.661334 120.661334-60.330667 60.330666-181.034667-60.330666 120.704-120.661334z\"/>"
    "</svg>");

static const QString TEXT_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M692.2597343921661 882.78857421875H331.711297750473v-30.689996480941772a563.3140236139297 563.3140236139297 0 0 1 48.088091611862176-14.83154296875c16.200274229049683-4.278552532196046 33.656305074691765-7.757592201232912 52.39561200141907-10.638445615768433v-630.340576171875h-136.73552870750427l-57.244107127189636 151.53810381889343H211.2339789867401c-2.1675199270248413-14.118209481239319-3.9642512798309326-30.233752727508545-5.419161915779114-48.23148250579833-1.4541864395141602-17.939794063568115-2.6809751987457275-36.30831241607666-3.7933409214019784-55.10410666465761-1.1123657226562498-18.59736442565918-1.9683659076690674-37.078857421875-2.6527315378189087-55.019375681877136C198.6264432668686 171.53063368797297 198.2563788890838 155.35860311985022 198.2563788890838 141.21142578125h627.4886906147003c0 14.147177338600159-0.3997564315795899 30.090361833572388-1.1123657226562498 47.689059376716614-0.7415771484374999 17.655909061431885-1.6547888517379765 35.852792859077454-2.766430377960205 54.56313192844391s-2.139276266098023 37.078857421875-3.251641988754273 55.10410666465761c-1.0841220617294314 17.998453974723816-2.7099430561065674 34.48406159877777-4.9064308404922485 49.28736090660096h-28.408053517341607l-56.75889551639557-151.53810381889343H592.8891206979752V826.6850764751434c18.71033906936645 3.5659432411193848 36.22285723686218 7.07395076751709 52.366644144058235 10.638445615768433 16.229242086410522 3.508731722831726 31.85957372188568 8.415162563323973 47.00469374656678 14.83154296875v30.633509159088135z\"/>"
    "</svg>");

static const QString ERASER_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M980.9 424.1c12.5-12.5 12.5-32.8 0-45.3L636.7 34.6c-12.5-12.5-32.8-12.5-45.3 0L90.5 535.5c-50 50-50 131 0 181l272 270.3c24 23.8 56.4 37.2 90.2 37.2H864c17.7 0 32-14.3 32-32 0-8.8-3.6-16.8-9.4-22.6-5.8-5.8-13.8-9.4-22.6-9.4H599.5c-57 0-85.6-68.9-45.3-109.3l426.7-426.6zM434.8 879.7c-25 25-65.5 25-90.5 0L135.8 671.3c-25-25-25-65.5 0-90.5l127.9-127.9 299 299-127.9 127.8z\"/>"
    "</svg>");

static const QString UNDO_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M507.426133 156.4672c-4.2496-0.119467-8.413867 0.119467-12.6464 0.136533V56.661333c0-27.392-30.651733-43.6224-53.316266-28.245333L82.756267 272.0256a34.133333 34.133333 0 0 0 0 56.4736l358.741333 243.592533c22.664533 15.394133 53.316267-0.836267 53.316267-28.245333v-101.0688c172.834133-2.0992 316.5696 118.545067 323.515733 274.551467 5.3248 119.671467-71.424 225.757867-185.207467 274.756266-2.065067 0.887467-4.232533 2.628267-4.0448 6.178134 0.1536 2.901333 3.106133 3.754667 5.7344 2.9696C816.930133 947.677867 951.005867 790.306133 956.074667 600.917333c6.365867-238.660267-194.491733-437.640533-448.648534-444.450133z\" fill=\"#040000\"/>"
    "</svg>");

static const QString REDO_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M1024 398.222222L568.888889 0v232.96C0 369.777778 0 1024 0 1024s199.111111-459.946667 568.888889-459.946667V796.444444z\"/>"
    "</svg>");

static const QString EXIT_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M86.016 0L2.048 70.656c149.504 111.616 288.768 239.616 411.136 367.616C225.792 627.2 78.848 813.056 1.536 887.808L161.28 1021.44c56.832-117.248 180.224-294.912 345.6-481.28C672.256 727.552 796.16 906.24 852.992 1024c0 0 155.648-165.376 169.472-139.776C962.56 816.64 816.128 620.032 619.52 418.816c112.64-115.712 239.104-230.4 374.272-331.264l-36.864-68.608c-153.088 76.288-299.008 189.44-430.08 309.76C394.752 203.264 245.248 83.968 86.016 0z\"/>"
    "</svg>");

static const QString PIN_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M414.245926 751.028148l437.57037-453.12c7.300741-7.49037 7.111111-19.531852-0.474074-26.832593-7.49037-7.300741-19.531852-7.111111-26.832593 0.474074L398.506667 712.722963 199.111111 513.327407c-7.395556-7.395556-19.437037-7.395556-26.832593 0l0 0c-7.395556 7.395556-7.395556 19.437037 0 26.832593l212.574815 212.574815c6.731852 6.731852 17.161481 7.300741 24.557037 1.896296C411.211852 753.682963 412.823704 752.545185 414.245926 751.028148z\" fill=\"#2c2c2c\"/>"
    "</svg>");

static const QString SAVE_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1127 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M989.541 940.667h-858.923c0 0-68.409 10.642-68.409-80.572v-737.306c0 0 1.521-82.091 85.132-82.091h301.003c0 0 36.486-7.601 66.89 39.525 28.884 45.607 45.607 74.491 45.607 74.491 0 0 10.642 12.161 34.965 12.161-21.283 0 387.655 0 387.655 0 0 0 68.409-7.601 68.409 68.409v629.371c0 0 10.642 76.012-62.33 76.012zM925.692 362.984c0-18.243-15.202-33.445-33.445-33.445h-668.896c-19.763 0-34.965 15.203-34.965 33.445v3.040c0 19.763 15.202 34.965 34.965 34.965h668.896c18.243 0 33.445-15.203 33.445-34.965v-3.040z\"/>"
    "</svg>");

static const QString COPY_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M37.786567 310.483897v551.803804a161.937525 161.937525 0 0 0 161.824912 161.712299h490.429708a161.937525 161.937525 0 0 0 161.824911-161.712299v-551.803804A161.937525 161.937525 0 0 0 690.041187 148.658985H199.611479A161.937525 161.937525 0 0 0 37.786567 310.483897z m78.829115 551.803804v-551.803804a83.446249 83.446249 0 0 1 83.446249-83.446249h489.979256a83.446249 83.446249 0 0 1 83.446248 83.446249v551.803804A83.558862 83.558862 0 0 1 690.041187 945.959176H199.611479A83.558862 83.558862 0 0 1 116.615682 862.287701z\" fill=\"#2c2c2c\"/>"
    "<path d=\"M199.611479 419.831141h499.889201a39.189331 39.189331 0 1 0 0-78.829115H199.611479a39.189331 39.189331 0 1 0 0 78.829115zM199.611479 631.318395h499.889201a39.189331 39.189331 0 1 0 0-78.829115H199.611479a39.189331 39.189331 0 1 0 0 78.829115zM199.611479 842.693036h506.758596a39.189331 39.189331 0 1 0 0-78.829115h-506.758596a39.189331 39.189331 0 1 0 0 78.829115z\" fill=\"#2c2c2c\"/>"
    "<path d=\"M983.735946 154.627476a182.207868 182.207868 0 0 0-18.468535-58.446158C930.132148 31.654056 859.861623 11.271099 841.730926 7.442257A355.519308 355.519308 0 0 0 781.820799 0.009797H449.837612a36.261393 36.261393 0 0 0-36.261393 28.491095 34.910037 34.910037 0 0 0 33.783907 41.103752h320.496658A148.874414 148.874414 0 0 1 916.168133 218.366446v451.352989a34.797424 34.797424 0 0 0 41.103753 33.783906 36.261393 36.261393 0 0 0 28.491095-36.261392V190.21319a244.820708 244.820708 0 0 0-2.252261-33.783906z\" fill=\"#2c2c2c\"/>"
    "</svg>");

static const QString COLOR_ICON_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M254.784 550.272a51.2 51.2 0 1 0-9.28-102.08 51.2 51.2 0 0 0 9.28 102.08z m144.256-268.544a64.192 64.192 0 0 0-59.712 59.712 64.128 64.128 0 0 0 68.096 68.096c31.808-1.984 57.6-27.84 59.648-59.712a64.128 64.128 0 0 0-68.032-68.096z m238.912-64a80 80 0 1 0 10.56 159.68 80 80 0 0 0-10.56-159.616zM995.2 217.6l-1.28-1.088a55.424 55.424 0 0 0-75.392 5.12L587.456 614.656 506.24 765.888l2.048 1.6-9.152 20.032 20.736-10.496 2.752 2.304 145.536-96 332.288-394.304a49.088 49.088 0 0 0-5.44-71.488zM920.96 524.928c-7.68 28.16-19.2 65.92-36.736 104a448.576 448.576 0 0 1-154.24 179.776c-80 53.696-138.496 71.04-190.08 71.04-40.96 0-80-6.976-112.64-20.224-29.824-12.032-51.456-28.16-60.864-45.44-3.84-6.976-8-14.72-12.416-23.104-19.072-35.584-36.352-78.208-50.88-90.688-17.6-15.104-43.328-17.216-66.304-17.216-8.96 0-45.184 1.536-53.76 1.536-36.608 0-54.72-7.488-62.08-25.6-19.776-48.448-14.08-169.984 23.68-249.088 58.496-122.24 157.824-209.728 282.56-243.264a511.936 511.936 0 0 1 134.272-19.008c74.88 0 123.776 10.112 190.336 43.136 17.856 8.832 65.536 34.944 80.704 45.696 12.672 8.96 32.064-0.768 41.856-12.992a31.104 31.104 0 0 0-7.552-45.568 1093.696 1093.696 0 0 0-95.616-53.888 481.856 481.856 0 0 0-209.728-48c-75.456 0-164.864 18.816-240.64 54.08A478.208 478.208 0 0 0 121.92 298.88c-40.32 58.88-74.88 141.568-84.992 244.992-3.904 40.768-7.488 65.728-2.624 100.48 4.672 33.408 14.848 60.096 29.824 77.248 24.832 28.8 57.024 42.816 98.56 42.816 15.424 0 62.464-5.76 69.952-5.76 8.256 0 13.568 1.408 16 4.416 7.04 8.448 16 25.6 26.24 45.632 8.32 16.128 17.6 34.432 28.16 51.84 13.44 22.336 40.896 46.272 73.664 64a307.712 307.712 0 0 0 146.56 35.456c79.744 0 164.352-27.008 251.2-80.256 83.008-50.752 136.32-136.064 166.4-198.656 21.12-43.84 38.528-99.84 48.64-136.32 5.248-19.776-10.688-41.344-30.464-43.456-25.92-7.168-34.048 8.384-38.144 23.68z\" fill=\"#000000\"/>"
    "</svg>");


static const QString MOSAIC_MODE_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M949.504 550.656L481.024 75.52c-15.232-15.488-39.808-15.488-55.04 0s-15.232 40.448 0 55.936l468.608 475.008c15.232 15.488 39.808 15.488 55.04 0 15.104-15.36 15.104-40.448-0.128-55.808zM130.432 418.304c-15.36-15.36-40.448-15.36-55.808 0s-15.36 40.448 0 55.808l474.88 474.368c15.36 15.36 40.448 15.36 55.808 0s15.36-40.448 0-55.808l-474.88-474.368zM183.552 133.12c-15.232-15.488-39.808-15.488-55.04 0s-15.232 40.448 0 55.936L820.864 892.16c15.232 15.488 39.808 15.488 55.04 0s15.232-40.448 0-55.936L183.552 133.12z\" fill=\"#2c2c2c\"/>"
    "</svg>");
static const QString BLUR_MODE_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M826.666667 160a32 32 0 0 1 31.850666 28.928L858.666667 192v640a32 32 0 0 1-28.928 31.850667l-3.072 0.149333h-640a32 32 0 0 1-31.850667-28.928L154.666667 832V192a32 32 0 0 1 28.928-31.850667L186.666667 160h640z m-32 64h-576v576h576v-576z m-26.752 415.082667v77.76a51.178667 51.178667 0 0 1-51.2 51.157333h-77.696l128.896-128.917333zM768 402.986667v103.850666L506.88 768h-108.693333l358.421333-358.4c3.2-3.157333 7.082667-5.418667 11.392-6.613333zM767.914667 256v44.970667L301.013333 768H256v-63.573333L729.941333 256h37.973334z m-172.522667 0c0.96 7.808-1.749333 15.637333-7.317333 21.184L256.085333 609.28v-108.629333H256L500.650667 256h94.741333z m-206.357333 0L256 389.12v-81.962667A51.114667 51.114667 0 0 1 307.2 256h81.834667z\" fill=\"#2c2c2c\"/>"
    "</svg>");
static const QString FILL_BOX_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<rect x=\"160\" y=\"256\" width=\"704\" height=\"512\" fill=\"#2c2c2c\" fill-opacity=\"0.4\" stroke=\"#2c2c2c\" stroke-width=\"20\"/>"
    "</svg>");
static const QString SKETCH_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M128 768 Q256 640 384 512 T640 256 L832 128\" fill=\"none\" stroke=\"#2c2c2c\" stroke-width=\"60\" stroke-linecap=\"round\"/>"
    "</svg>");
static const QString HL_SKETCH_SVG = SKETCH_SVG;
static const QString HL_BOX_SVG = FILL_BOX_SVG;
static const QString ERASER_FREE_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<circle cx=\"256\" cy=\"768\" r=\"100\" fill=\"#2c2c2c\" fill-opacity=\"0.5\" stroke=\"#2c2c2c\" stroke-width=\"10\"/>"
    "<circle cx=\"512\" cy=\"512\" r=\"100\" fill=\"#2c2c2c\" fill-opacity=\"0.5\" stroke=\"#2c2c2c\" stroke-width=\"10\"/>"
    "<circle cx=\"768\" cy=\"256\" r=\"100\" fill=\"#2c2c2c\" fill-opacity=\"0.5\" stroke=\"#2c2c2c\" stroke-width=\"10\"/>"
    "</svg>");
static const QString ERASER_FILL_SVG = QStringLiteral(
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<rect x=\"160\" y=\"160\" width=\"704\" height=\"704\" fill=\"#2c2c2c\" fill-opacity=\"0.3\" stroke=\"#2c2c2c\" stroke-width=\"20\" stroke-dasharray=\"40 20\"/>"
    "</svg>");


static QIcon renderSvgIcon(const QString& svgStr, int size = 20) {
    bool dark = UiTheme::is_dark_theme();
    
    QString cacheKey = svgStr + QStringLiteral("|%1|%2").arg(size).arg(dark ? 1 : 0);
    static QHash<QString, QIcon> s_cache;
    auto it = s_cache.constFind(cacheKey);
    if (it != s_cache.cend()) return *it;

    QIcon icon;
    static const QRegularExpression re(QStringLiteral(" fill=\"[^\"]*\""));
    QString svgBase = svgStr;
    svgBase.replace(re, QString());

    QString normalColor = dark ? QStringLiteral("#ffffff") : QStringLiteral("#2c2c2c");

    
    QString svgNormal = svgBase;
    svgNormal.replace(QStringLiteral("<path"), QStringLiteral("<path fill=\"%1\"").arg(normalColor));
    svgNormal.replace(QStringLiteral("<g"), QStringLiteral("<g fill=\"%1\"").arg(normalColor));
    {
        QSvgRenderer renderer(QByteArray(svgNormal.toUtf8()));
        if (renderer.isValid()) {
            QPixmap pm(QSize(size, size));
            pm.fill(Qt::transparent);
            QPainter pp(&pm);
            pp.setRenderHint(QPainter::Antialiasing, true);
            pp.setRenderHint(QPainter::SmoothPixmapTransform, true);
            renderer.render(&pp, QRectF(0, 0, size, size));
            pp.end();
            icon.addPixmap(pm, QIcon::Normal, QIcon::Off);
            icon.addPixmap(pm, QIcon::Active, QIcon::Off);
        }
    }
    
    QString svgWhite = svgBase;
    svgWhite.replace(QStringLiteral("<path"), QStringLiteral("<path fill=\"#ffffff\""));
    svgWhite.replace(QStringLiteral("<g"), QStringLiteral("<g fill=\"#ffffff\""));
    {
        QSvgRenderer renderer(QByteArray(svgWhite.toUtf8()));
        if (renderer.isValid()) {
            QPixmap pm(QSize(size, size));
            pm.fill(Qt::transparent);
            QPainter pp(&pm);
            pp.setRenderHint(QPainter::Antialiasing, true);
            pp.setRenderHint(QPainter::SmoothPixmapTransform, true);
            renderer.render(&pp, QRectF(0, 0, size, size));
            pp.end();
            icon.addPixmap(pm, QIcon::Normal, QIcon::On);
            icon.addPixmap(pm, QIcon::Selected, QIcon::Off);
        }
    }
    s_cache.insert(cacheKey, icon);
    return icon;
}


static void drawArrowShape(QPainter& painter, const QPointF& start, const QPointF& end,
                           const QColor& color, int width, ArrowStyle style) {
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::hypot(dx, dy);
    if (length < 1e-6) return;
    double ux = dx / length, uy = dy / length;
    double nx = -uy, ny = ux;

    auto maxd = [](double a, double b) { return a > b ? a : b; };
    auto mind = [](double a, double b) { return a < b ? a : b; };

    if (style == ArrowStyle::Line) {
        QPen pen(color, maxd(width, 2));
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(start, end);
        return;
    }
    if (style == ArrowStyle::Simple) {
        QPen pen(color, maxd(width, 2));
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        double scale = maxd(width / 4.0, 0.5);
        double headLen = mind(18.0 * scale, length * 0.3);
        double headW = 14.0 * scale;
        double hbx = end.x() - ux * headLen;
        double hby = end.y() - uy * headLen;
        QPointF lineEnd(hbx, hby);
        painter.drawLine(start, lineEnd);
        double half = headW / 2;
        QPointF wl(hbx + nx * half, hby + ny * half);
        QPointF wr(hbx - nx * half, hby - ny * half);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(QPolygonF() << wl << end << wr);
        return;
    }
    // Taper / Hollow
    double scale = maxd(width / 4.0, 0.5);
    double tailW = 1.0 * scale;
    double maxShaftW = 7.0 * scale;
    double headW = 14.0 * scale;
    double headLen = mind(18.0 * scale, length * 0.4);
    double wingCurve = headLen * 0.12;
    double hbx = end.x() - ux * headLen;
    double hby = end.y() - uy * headLen;
    double halfTail = tailW / 2, halfShaft = maxShaftW / 2, halfHead = headW / 2;
    QPointF tailL(start.x() + nx * halfTail, start.y() + ny * halfTail);
    QPointF tailR(start.x() - nx * halfTail, start.y() - ny * halfTail);
    QPointF shaftL(hbx + nx * halfShaft, hby + ny * halfShaft);
    QPointF shaftR(hbx - nx * halfShaft, hby - ny * halfShaft);
    QPointF wingL(hbx + nx * halfHead - ux * wingCurve, hby + ny * halfHead - uy * wingCurve);
    QPointF wingR(hbx - nx * halfHead - ux * wingCurve, hby - ny * halfHead - uy * wingCurve);
    if (style == ArrowStyle::Hollow) {
        painter.setBrush(Qt::NoBrush);
        QPen pen(color, maxd(width * 0.5, 1.2));
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
    } else {
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
    }
    painter.drawPolygon(QPolygonF() << tailL << shaftL << wingL << end << wingR << shaftR << tailR);
}


static QIcon makeArrowIcon(ArrowStyle style, int size = 24) {
    QIcon icon;
    auto build = [&](const QColor& c) {
        QPixmap pm(QSize(size, size));
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPoint start(4, size / 2);  
        QPoint end(size - 5, size / 2);  
        drawArrowShape(p, start, end, c, 4, style);
        p.end();
        return pm;
    };
    bool dark = UiTheme::is_dark_theme();
    QColor normal = dark ? QColor("#ffffff") : QColor("#2c2c2c");
    icon.addPixmap(build(normal), QIcon::Normal, QIcon::Off);
    icon.addPixmap(build(normal), QIcon::Active, QIcon::Off);
    QPixmap white = build(QColor("#ffffff"));
    icon.addPixmap(white, QIcon::Normal, QIcon::On);
    icon.addPixmap(white, QIcon::Selected, QIcon::Off);
    return icon;
}

// ==================== ColorPanel ====================
// Snipaste-style compact horizontal color strip; embeds into StickerToolbar when parent != nullptr.
class ColorPanel : public QWidget {
    Q_OBJECT
public:
    ColorPanel(QWidget* parent = nullptr, const QColor& current = QColor(255, 0, 0))
        : QWidget(parent), m_current(current) {
        if (!parent) {
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            setAttribute(Qt::WA_ShowWithoutActivating);
        }
        // ColorPanel merges into the toolbar's translucent panel; no own bg.
        // 2-row compact grid: 15 colors + 1 custom = 16 cells (2 x 8)
        auto* grid = new QGridLayout(this);
        grid->setContentsMargins(2, 2, 2, 2);
        grid->setSpacing(2);
        const QStringList colors = {
            "#FF0000","#FF4500","#FF8C00","#FFD700","#9ACD32",
            "#00CC00","#00CED1","#1E90FF","#4169E1","#8A2BE2",
            "#FF1493","#8B4513","#000000","#FFFFFF","#808080"
        };
        int idx = 0;
        for (const auto& c : colors) {
            auto* btn = new QPushButton();
            btn->setFixedSize(18, 18);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(QString("background: %1; border: 1px solid #888; border-radius: 3px; padding: 0;").arg(c));
            connect(btn, &QPushButton::clicked, this, [this, c](){ emit colorPicked(QColor(c)); });
            grid->addWidget(btn, idx / 8, idx % 8);
            ++idx;
        }
        auto* custom = new QPushButton("...");
        custom->setFixedSize(18, 18);
        custom->setCursor(Qt::PointingHandCursor);
        custom->setToolTip(I18n::t("sticker.select_color", QStringLiteral("选择颜色")));
        connect(custom, &QPushButton::clicked, this, [this](){
            QColorDialog dlg(m_current, this);
            dlg.setWindowTitle(I18n::t("sticker.select_color", QStringLiteral("选择颜色")));
            dlg.setOption(QColorDialog::DontUseNativeDialog, true);
            QColor c = dlg.exec() == QDialog::Accepted ? dlg.selectedColor() : QColor();
            if (c.isValid()) emit colorPicked(c);
        });
        grid->addWidget(custom, idx / 8, idx % 8);
        m_customBtn = custom;
        grid->activate();
        adjustSize();
    }
    void refreshLanguage() {
        if (m_customBtn) m_customBtn->setToolTip(I18n::t("sticker.select_color", QStringLiteral("选择颜色")));
    }
signals:
    void colorPicked(const QColor& c);
private:
    QColor m_current;
    QPushButton* m_customBtn = nullptr;
};

// ==================== FontPanel ====================
// Snipaste-style compact single-row: [B][I] [Font ▾] [Size:16]; embeds when parent != nullptr.
class FontPanel : public QWidget {
    Q_OBJECT
public:
    FontPanel(QWidget* parent = nullptr, const QFont& current = QFont("Microsoft YaHei", 16))
        : QWidget(parent), m_font(current) {
        if (!parent) {
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            setAttribute(Qt::WA_ShowWithoutActivating);
        }
        applyStyle();

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(4, 2, 4, 2);
        row->setSpacing(3);

        // Bold toggle (Normal ↔ Bold only; 5-step weight dropped per Snipaste style)
        m_boldBtn = new QToolButton(this);
        m_boldBtn->setText(QStringLiteral("B"));
        m_boldBtn->setCheckable(true);
        m_boldBtn->setChecked(current.weight() >= QFont::Bold);
        m_boldBtn->setFixedWidth(26);
        m_boldBtn->setToolTip(I18n::t("sticker.weight_bold", QStringLiteral("粗体")));
        row->addWidget(m_boldBtn);

        // Italic toggle
        m_italicBtn = new QToolButton(this);
        m_italicBtn->setText(QStringLiteral("I"));
        m_italicBtn->setCheckable(true);
        m_italicBtn->setChecked(current.italic());
        m_italicBtn->setFixedWidth(26);
        QFont btnFont = m_italicBtn->font();
        btnFont.setItalic(true);
        m_italicBtn->setFont(btnFont);
        m_italicBtn->setToolTip(I18n::t("sticker.italic", QStringLiteral("斜体")));
        row->addWidget(m_italicBtn);
        m_boldTip = {"sticker.weight_bold", QStringLiteral("粗体")};
        m_italicTip = {"sticker.italic", QStringLiteral("斜体")};

        // Font family dropdown
        m_family = new QComboBox(this);
        m_family->setMaxVisibleItems(12);
        QStringList commonFonts = {
            "Microsoft YaHei", "SimSun", "SimHei", "KaiTi", "FangSong",
            "Arial", "Times New Roman", "Courier New", "Calibri",
            "Consolas", "Verdana", "Tahoma"
        };
        for (const auto& f : commonFonts) m_family->addItem(f);
        QFontDatabase db;
        for (const auto& f : db.families()) {
            if (!commonFonts.contains(f)) m_family->addItem(f);
        }
        int idx = m_family->findText(current.family());
        if (idx >= 0) m_family->setCurrentIndex(idx);
        row->addWidget(m_family, 1);

        // Font size
        m_size = new QSpinBox(this);
        m_size->setRange(8, 200);
        m_size->setValue(current.pointSize() > 0 ? current.pointSize() : 16);
        row->addWidget(m_size);

        connect(m_family, &QComboBox::currentTextChanged, this, [this](){ syncFont(); });
        connect(m_boldBtn, &QToolButton::toggled, this, [this](){ syncFont(); });
        connect(m_italicBtn, &QToolButton::toggled, this, [this](){ syncFont(); });
        connect(m_size, qOverload<int>(&QSpinBox::valueChanged), this, [this](){ syncFont(); });
    }
    void applyStyle() {
        bool dark = UiTheme::is_dark_theme();
        QString border = dark ? "#3d3d3d" : "#cccccc";
        QString text = dark ? "#ffffff" : "#333333";
        QString inputBg = dark ? "#404040" : "#ffffff";
        setStyleSheet(QString(
            "FontPanel { background: transparent; }"
            "QLabel { color: %2; font-size: 12px; }"
            "QComboBox { background: %3; color: %2; border: 1px solid %1; border-radius: 3px; padding: 2px 6px; min-width: 100px; max-width: 160px; }"
            "QComboBox QAbstractItemView { background: %3; color: %2; selection-background-color: #1a7eff; selection-color: #ffffff; outline: none; border: 1px solid %1; }"
            "QSpinBox { background: %3; color: %2; border: 1px solid %1; border-radius: 3px; padding: 2px 4px; max-width: 60px; }"
            "QToolButton { background: transparent; border: 1px solid %1; border-radius: 3px; padding: 2px 6px; color: %2; font-size: 12px; font-weight: bold; }"
            "QToolButton:checked { background: #1a7eff; color: white; border-color: #1a7eff; }"
        ).arg(border, text, inputBg));
    }
    void syncFont() {
        QString family = m_family->currentText();
        int weight = (int)(m_boldBtn->isChecked() ? QFont::Bold : QFont::Normal);
        bool italic = m_italicBtn->isChecked();
        m_font = QFont(family, m_size->value());
        m_font.setWeight((QFont::Weight)weight);
        m_font.setItalic(italic);
        emit fontChanged(m_font);
    }
    QFont getFont() const {
        QFont f(m_family->currentText(), m_size->value());
        f.setWeight(m_boldBtn->isChecked() ? QFont::Bold : QFont::Normal);
        f.setItalic(m_italicBtn->isChecked());
        return f;
    }
    
    void setPointSize(int sz) {
        sz = qBound(8, sz, 200);
        if (m_size->value() != sz) {
            m_size->setValue(sz);  
        }
    }
    void refreshLanguage() {
        if (m_boldBtn) m_boldBtn->setToolTip(I18n::t(m_boldTip.first, m_boldTip.second));
        if (m_italicBtn) m_italicBtn->setToolTip(I18n::t(m_italicTip.first, m_italicTip.second));
    }
signals:
    void fontChanged(const QFont& f);
private:
    QPair<QString,QString> m_boldTip, m_italicTip;
    QFont m_font;
    QComboBox* m_family = nullptr;
    QToolButton* m_boldBtn = nullptr;
    QToolButton* m_italicBtn = nullptr;
    QSpinBox* m_size = nullptr;
};

// ==================== ShapeTypePanel ====================
class ShapeTypePanel : public QWidget {
    Q_OBJECT
public:
    ShapeTypePanel(QWidget* parent = nullptr, Tool currentTool = Tool::Rect, bool currentFill = false)
        : QWidget(parent) {
        if (!parent) {
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            setAttribute(Qt::WA_ShowWithoutActivating);
        }
        applyStyle();
        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(4, 4, 4, 4);
        lay->setSpacing(2);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        struct Opt { Tool tool; bool fill; QString svg; QString tipKey; QString label; };
        QVector<Opt> opts = {
            {Tool::Rect, false, RECT_HOLLOW_SVG, "sticker.rect_hollow", "矩形镂空"},
            {Tool::Rect, true, RECT_FILL_SVG, "sticker.rect_fill", "矩形填充"},
            {Tool::Ellipse, false, ELLIPSE_HOLLOW_SVG, "sticker.ellipse_hollow", "椭圆镂空"},
            {Tool::Ellipse, true, ELLIPSE_FILL_SVG, "sticker.ellipse_fill", "椭圆填充"},
        };
        for (const auto& o : opts) {
            auto* btn = new QToolButton();
            btn->setFixedSize(28, 26);
            btn->setCheckable(true);
            btn->setChecked(o.tool == currentTool && o.fill == currentFill);
            btn->setToolTip(I18n::t(o.tipKey, o.label));
            btn->setToolTipDuration(5000);
            QIcon ic = renderSvgIcon(o.svg, 20);
            if (!ic.isNull()) { btn->setIcon(ic); btn->setIconSize(QSize(20, 20)); }
            Tool t = o.tool; bool f = o.fill;
            connect(btn, &QToolButton::clicked, this, [this, t, f](){ emit shapeSelected(t, f); });
            lay->addWidget(btn);
            m_buttons.append(btn);
            m_svgs.append(o.svg);
            m_tipKeys.append(o.tipKey);
            m_labels.append(o.label);
        }
    }
    void refreshLanguage() {
        for (int i = 0; i < m_buttons.size() && i < m_tipKeys.size(); ++i)
            m_buttons[i]->setToolTip(I18n::t(m_tipKeys[i], m_labels[i]));
    }
    void refreshIcons() {
        for (int i = 0; i < m_buttons.size() && i < m_svgs.size(); ++i) {
            QIcon ic = renderSvgIcon(m_svgs[i], 20);
            if (!ic.isNull()) m_buttons[i]->setIcon(ic);
        }
    }
    void setCurrent(Tool tool, bool fill) {
        QVector<QPair<Tool,bool>> defs = {
            {Tool::Rect, false}, {Tool::Rect, true},
            {Tool::Ellipse, false}, {Tool::Ellipse, true}
        };
        for (int i = 0; i < m_buttons.size() && i < defs.size(); ++i)
            m_buttons[i]->setChecked(defs[i].first == tool && defs[i].second == fill);
    }
    void applyStyle() {
        bool dark = UiTheme::is_dark_theme();
        QString bg = dark ? "#2d2d2d" : "#ffffff";
        QString border = dark ? "#3d3d3d" : "#cccccc";
        QString hover = dark ? "#404040" : "#e0e0e0";
        setStyleSheet(QString(
            "ShapeTypePanel { background: %1; border: 1px solid %2; border-radius: 6px; }"
            "QToolButton { background: transparent; border: none; border-radius: 4px; padding: 2px; }"
            "QToolButton:hover { background: %3; }"
            "QToolButton:checked { background: #1a7eff; }"
        ).arg(bg, border, hover));
        refreshIcons();
    }
signals:
    void shapeSelected(Tool tool, bool fill);
private:
    QList<QToolButton*> m_buttons;
    QStringList m_svgs;
    QStringList m_tipKeys;
    QStringList m_labels;
};

// ==================== ArrowTypePanel ====================
class ArrowTypePanel : public QWidget {
    Q_OBJECT
public:
    ArrowTypePanel(QWidget* parent = nullptr, ArrowStyle currentStyle = ArrowStyle::Taper)
        : QWidget(parent) {
        if (!parent) {
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            setAttribute(Qt::WA_ShowWithoutActivating);
        }
        applyStyle();
        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(4, 4, 4, 4);
        lay->setSpacing(2);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        struct Opt { ArrowStyle style; QString tipKey; QString label; bool isPng; };
        QVector<Opt> opts = {
            {ArrowStyle::Line, "sticker.arrow_line", "直线", false},
            {ArrowStyle::Simple, "sticker.arrow_simple", "普通箭头", false},
            {ArrowStyle::Taper, "sticker.arrow_taper", "锥形箭头", true},
            {ArrowStyle::Hollow, "sticker.arrow_hollow", "锥形镂空", true},
        };
        for (const auto& o : opts) {
            auto* btn = new QToolButton();
            btn->setFixedSize(40, 36);
            btn->setCheckable(true);
            btn->setChecked(o.style == currentStyle);
            btn->setToolTip(I18n::t(o.tipKey, o.label));
            btn->setToolTipDuration(5000);
            QIcon ic = o.isPng ? makeArrowIcon(o.style, 24) : renderSvgIcon(
                o.style == ArrowStyle::Line ? LINE_ICON_SVG : ARROW_SIMPLE_SVG, 24);
            if (!ic.isNull()) { btn->setIcon(ic); btn->setIconSize(QSize(24, 24)); }
            ArrowStyle s = o.style;
            connect(btn, &QToolButton::clicked, this, [this, s](){ emit arrowSelected(s); });
            lay->addWidget(btn);
            m_buttons.append(btn);
            m_styles.append(o.style);
            m_isPng.append(o.isPng);
            m_tipKeys.append(o.tipKey);
            m_labels.append(o.label);
        }
    }
    void refreshLanguage() {
        for (int i = 0; i < m_buttons.size() && i < m_tipKeys.size(); ++i)
            m_buttons[i]->setToolTip(I18n::t(m_tipKeys[i], m_labels[i]));
    }
    void refreshIcons() {
        for (int i = 0; i < m_buttons.size() && i < m_styles.size(); ++i) {
            QIcon ic = m_isPng[i] ? makeArrowIcon(m_styles[i], 24) : renderSvgIcon(
                m_styles[i] == ArrowStyle::Line ? LINE_ICON_SVG : ARROW_SIMPLE_SVG, 24);
            if (!ic.isNull()) m_buttons[i]->setIcon(ic);
        }
    }
    void setCurrent(ArrowStyle style) {
        QVector<ArrowStyle> defs = {ArrowStyle::Line, ArrowStyle::Simple, ArrowStyle::Taper, ArrowStyle::Hollow};
        for (int i = 0; i < m_buttons.size() && i < defs.size(); ++i)
            m_buttons[i]->setChecked(defs[i] == style);
    }
    void applyStyle() {
        bool dark = UiTheme::is_dark_theme();
        QString bg = dark ? "#2d2d2d" : "#ffffff";
        QString border = dark ? "#3d3d3d" : "#cccccc";
        QString hover = dark ? "#404040" : "#e0e0e0";
        setStyleSheet(QString(
            "ArrowTypePanel { background: %1; border: 1px solid %2; border-radius: 6px; }"
            "QToolButton { background: transparent; border: none; border-radius: 4px; padding: 2px; }"
            "QToolButton:hover { background: %3; }"
            "QToolButton:checked { background: #1a7eff; }"
        ).arg(bg, border, hover));
        refreshIcons();
    }
signals:
    void arrowSelected(ArrowStyle style);
private:
    QList<QToolButton*> m_buttons;
    QVector<ArrowStyle> m_styles;
    QVector<bool> m_isPng;
    QStringList m_tipKeys;
    QStringList m_labels;
};

// ==================== MosaicPanel ====================
class MosaicPanel : public QWidget {
    Q_OBJECT
public:
    MosaicPanel(QWidget* parent = nullptr, int currentBlock = 10,
                const QString& currentMode = "mosaic", bool currentFill = false,
                int currentOpacity = 6)
        : QWidget(parent) {
        if (!parent) {
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            setAttribute(Qt::WA_ShowWithoutActivating);
        }
        m_currentMode = currentMode;
        applyStyle();
        setFixedHeight(30);
        setMinimumWidth(220);
        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(6, 2, 6, 2);
        lay->setSpacing(5);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        
        m_btnBox = new QToolButton();
        m_btnBox->setFixedSize(28, 26); m_btnBox->setCheckable(true);
        m_btnBox->setChecked(currentFill);
        m_btnBox->setToolTip(I18n::t("sticker.mosaic_box", "框选"));
        m_btnBox->setToolTipDuration(5000);
        QIcon boxIcon = renderSvgIcon(FILL_BOX_SVG, 20);
        if (!boxIcon.isNull()) { m_btnBox->setIcon(boxIcon); m_btnBox->setIconSize(QSize(20, 20)); }
        connect(m_btnBox, &QToolButton::clicked, this, [this](bool checked){ emit fillChanged(checked); });
        lay->addWidget(m_btnBox);

        
        m_btnMosaic = mkModeBtn(currentMode == "mosaic", I18n::t("sticker.mosaic_mode", "马赛克"),
                                renderSvgIcon(MOSAIC_MODE_SVG, 20));
        m_btnBlur = mkModeBtn(currentMode == "blur", I18n::t("sticker.blur_mode", "模糊"),
                              renderSvgIcon(BLUR_MODE_SVG, 20));
        auto* modeGroup = new QButtonGroup(this);
        modeGroup->setExclusive(true);
        modeGroup->addButton(m_btnMosaic);
        modeGroup->addButton(m_btnBlur);
        connect(m_btnMosaic, &QToolButton::clicked, this, [this](){ m_currentMode = "mosaic"; emit modeChanged("mosaic"); });
        connect(m_btnBlur, &QToolButton::clicked, this, [this](){ m_currentMode = "blur"; emit modeChanged("blur"); });
        lay->addWidget(m_btnMosaic);
        lay->addWidget(m_btnBlur);

        auto* sep = new QFrame(); sep->setFrameShape(QFrame::VLine); sep->setFixedWidth(1);
        sep->setStyleSheet("color: #888;"); lay->addWidget(sep);

        
        m_slider = new QSlider(Qt::Horizontal);
        m_slider->setRange(6, 24);
        m_slider->setValue(currentOpacity);
        m_slider->setFixedWidth(80);
        m_slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_slider->setStyleSheet(
            "QSlider::grove:horizontal { border: none; height: 6px; background: #d0d0d0; border-radius: 3px; }"
            "QSlider::handle:horizontal { border: none; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; background: #555555; }"
            "QSlider::sub-page:horizontal { background: #999999; border-radius: 3px; }"
            "QSlider::add-page:horizontal { background: #d0d0d0; border-radius: 3px; }"
        );
        connect(m_slider, &QSlider::valueChanged, this, [this](int v){
            m_valueLabel->setText(QString::number(v));
            emit opacityChanged(v);
        });
        lay->addWidget(m_slider);
        lay->addSpacing(8);
        m_valueLabel = new QLabel(QString::number(currentOpacity));
        m_valueLabel->setObjectName("mosaicValue");
        m_valueLabel->setFixedSize(28, 22);
        m_valueLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        m_valueLabel->setStyleSheet("QLabel#mosaicValue { color: white; font-size: 12px; font-weight: bold; background: #1a7eff; border-radius: 4px; }");
        lay->addWidget(m_valueLabel);
        lay->activate();
        adjustSize();
    }
    QToolButton* mkModeBtn(bool checked, const QString& tip, const QIcon& ic) {
        auto* b = new QToolButton();
        b->setFixedSize(28, 26); b->setCheckable(true); b->setChecked(checked);
        b->setToolTip(tip);
        if (!ic.isNull()) { b->setIcon(ic); b->setIconSize(QSize(20, 20)); }
        return b;
    }
    void applyStyle() {
        bool dark = UiTheme::is_dark_theme();
        QString bg = dark ? "#2d2d2d" : "#ffffff";
        QString border = dark ? "#3d3d3d" : "#cccccc";
        QString hover = dark ? "#404040" : "#e0e0e0";
        setStyleSheet(QString(
            "MosaicPanel { background: transparent; }"
            "QSlider { background: transparent; }"
            "QToolButton { background: transparent; border: none; border-radius: 4px; padding: 1px; }"
            "QToolButton:hover { background: %3; }"
            "QToolButton:checked { background: #1a7eff; }"
        ).arg(bg, border, hover));
        if (m_btnBox) { QIcon ic = renderSvgIcon(FILL_BOX_SVG, 20); if (!ic.isNull()) m_btnBox->setIcon(ic); }
        if (m_btnMosaic) { QIcon ic = renderSvgIcon(MOSAIC_MODE_SVG, 20); if (!ic.isNull()) m_btnMosaic->setIcon(ic); }
        if (m_btnBlur) { QIcon ic = renderSvgIcon(BLUR_MODE_SVG, 20); if (!ic.isNull()) m_btnBlur->setIcon(ic); }
    }
    void refreshLanguage() {
        if (m_btnBox) m_btnBox->setToolTip(I18n::t("sticker.mosaic_box", "框选"));
        if (m_btnMosaic) m_btnMosaic->setToolTip(I18n::t("sticker.mosaic_mode", "马赛克"));
        if (m_btnBlur) m_btnBlur->setToolTip(I18n::t("sticker.blur_mode", "模糊"));
    }
    void refreshIcons() {
        if (m_btnBox) { QIcon ic = renderSvgIcon(FILL_BOX_SVG, 20); if (!ic.isNull()) m_btnBox->setIcon(ic); }
        if (m_btnMosaic) { QIcon ic = renderSvgIcon(MOSAIC_MODE_SVG, 20); if (!ic.isNull()) m_btnMosaic->setIcon(ic); }
        if (m_btnBlur) { QIcon ic = renderSvgIcon(BLUR_MODE_SVG, 20); if (!ic.isNull()) m_btnBlur->setIcon(ic); }
    }
    void setFill(bool fill) {
        if (m_btnBox) { QSignalBlocker b(m_btnBox); m_btnBox->setChecked(fill); }
    }
    void setOpacity(int op) {
        QSignalBlocker blocker(m_slider);
        m_slider->setValue(op);
        if (m_valueLabel) m_valueLabel->setText(QString::number(op));
    }
signals:
    void fillChanged(bool fill);
    void modeChanged(const QString& mode);
    void opacityChanged(int opacity);
private:
    QToolButton* m_btnBox = nullptr;
    QToolButton* m_btnMosaic = nullptr;
    QToolButton* m_btnBlur = nullptr;
    QSlider* m_slider = nullptr;
    QLabel* m_valueLabel = nullptr;
    QString m_currentMode = "mosaic";
};

// ==================== HighlighterTypePanel ====================
class HighlighterTypePanel : public QWidget {
    Q_OBJECT
public:
    HighlighterTypePanel(QWidget* parent = nullptr, const QString& currentMode = "sketch")
        : QWidget(parent) {
        if (!parent) {
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            setAttribute(Qt::WA_ShowWithoutActivating);
        }
        applyStyle();
        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(4, 4, 4, 4);
        lay->setSpacing(2);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        
        m_btnBox = new QToolButton();
        m_btnBox->setFixedSize(30, 30); m_btnBox->setCheckable(true);
        m_btnBox->setChecked(currentMode == "box");
        m_btnBox->setToolTip(I18n::t("sticker.hl_box", "框选"));
        m_btnBox->setToolTipDuration(5000);
        QIcon ic = renderSvgIcon(HL_BOX_SVG, 24);
        if (!ic.isNull()) { m_btnBox->setIcon(ic); m_btnBox->setIconSize(QSize(24, 24)); }
        connect(m_btnBox, &QToolButton::clicked, this, [this](bool checked){
            emit hlSelected(checked ? "box" : "sketch");
        });
        lay->addWidget(m_btnBox);
    }
    void refreshLanguage() {
        if (m_btnBox) m_btnBox->setToolTip(I18n::t("sticker.hl_box", "框选"));
    }
    void refreshIcons() {
        if (m_btnBox) { QIcon ic = renderSvgIcon(HL_BOX_SVG, 24); if (!ic.isNull()) m_btnBox->setIcon(ic); }
    }
    void setCurrent(const QString& mode) {
        if (m_btnBox) { QSignalBlocker b(m_btnBox); m_btnBox->setChecked(mode == "box"); }
    }
    void applyStyle() {
        bool dark = UiTheme::is_dark_theme();
        QString bg = dark ? "#2d2d2d" : "#ffffff";
        QString border = dark ? "#3d3d3d" : "#cccccc";
        QString hover = dark ? "#404040" : "#e0e0e0";
        setStyleSheet(QString(
            "HighlighterTypePanel { background: %1; border: 1px solid %2; border-radius: 6px; }"
            "QToolButton { background: transparent; border: none; border-radius: 4px; padding: 2px; }"
            "QToolButton:hover { background: %3; }"
            "QToolButton:checked { background: #1a7eff; }"
        ).arg(bg, border, hover));
        refreshIcons();
    }
signals:
    void hlSelected(const QString& mode);
private:
    QToolButton* m_btnBox = nullptr;
};

// ==================== EraserTypePanel ====================
class EraserTypePanel : public QWidget {
    Q_OBJECT
public:
    EraserTypePanel(QWidget* parent = nullptr, const QString& currentMode = "free")
        : QWidget(parent) {
        if (!parent) {
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            setAttribute(Qt::WA_ShowWithoutActivating);
        }
        applyStyle();
        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(4, 4, 4, 4);
        lay->setSpacing(2);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        struct Opt { QString mode; QString svg; QString tipKey; QString label; };
        QVector<Opt> opts = {
            {"fill", ERASER_FILL_SVG, "sticker.eraser_fill", "填充擦除"},
        };
        for (const auto& o : opts) {
            auto* btn = new QToolButton();
            btn->setFixedSize(30, 30); btn->setCheckable(true);
            btn->setChecked(o.mode == currentMode);
            btn->setToolTip(I18n::t(o.tipKey, o.label));
            btn->setToolTipDuration(5000);
            QIcon ic = renderSvgIcon(o.svg, 24);
            if (!ic.isNull()) { btn->setIcon(ic); btn->setIconSize(QSize(24, 24)); }
            QString m = o.mode;
            connect(btn, &QToolButton::clicked, this, [this, m](){ emit eraserSelected(m); });
            lay->addWidget(btn);
            m_buttons.append(btn);
            m_svgs.append(o.svg);
            m_tipKeys.append(o.tipKey);
            m_labels.append(o.label);
        }
    }
    void refreshLanguage() {
        for (int i = 0; i < m_buttons.size() && i < m_tipKeys.size(); ++i)
            m_buttons[i]->setToolTip(I18n::t(m_tipKeys[i], m_labels[i]));
    }
    void refreshIcons() {
        for (int i = 0; i < m_buttons.size() && i < m_svgs.size(); ++i) {
            QIcon ic = renderSvgIcon(m_svgs[i], 24);
            if (!ic.isNull()) m_buttons[i]->setIcon(ic);
        }
    }
    void setCurrent(const QString& mode) {
        QStringList defs = {"fill"};
        for (int i = 0; i < m_buttons.size() && i < defs.size(); ++i)
            m_buttons[i]->setChecked(defs[i] == mode);
    }
    void applyStyle() {
        bool dark = UiTheme::is_dark_theme();
        QString bg = dark ? "#2d2d2d" : "#ffffff";
        QString border = dark ? "#3d3d3d" : "#cccccc";
        QString hover = dark ? "#404040" : "#e0e0e0";
        setStyleSheet(QString(
            "EraserTypePanel { background: %1; border: 1px solid %2; border-radius: 6px; }"
            "QToolButton { background: transparent; border: none; border-radius: 4px; padding: 2px; }"
            "QToolButton:hover { background: %3; }"
            "QToolButton:checked { background: #1a7eff; }"
        ).arg(bg, border, hover));
        refreshIcons();
    }
signals:
    void eraserSelected(const QString& mode);
private:
    QList<QToolButton*> m_buttons;
    QStringList m_svgs;
    QStringList m_tipKeys;
    QStringList m_labels;
};

// ==================== PenPanel ====================

class PenPanel : public QWidget {
    Q_OBJECT
public:
    PenPanel(QWidget* parent = nullptr, int width = 3)
        : QWidget(parent) {
        Q_UNUSED(width)
        if (!parent) {
            setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            setAttribute(Qt::WA_ShowWithoutActivating);
        }
        setFixedSize(0, 0);
        applyStyle();
    }
    void applyStyle() {
        setStyleSheet("PenPanel { background: transparent; }");
    }
signals:
    void widthChanged(int w);
};

// ==================== WidthDotPreview ====================

class WidthDotPreview : public QWidget {
    Q_OBJECT
public:
    explicit WidthDotPreview(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedHeight(30);
        setFixedWidth(30);
        setVisible(false);
        setCursor(Qt::PointingHandCursor);
    }
    void setInfo(const QString& toolName, int width) {
        m_toolName = toolName;
        m_width = width;
        setToolTip(QStringLiteral("%1 %2px").arg(toolName).arg(width));
        update();
    }
    void setSelected(bool sel) {
        if (m_selected != sel) { m_selected = sel; update(); }
    }
    bool isSelected() const { return m_selected; }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRect r = rect().adjusted(1, 1, -1, -1);
        p.setPen(Qt::NoPen);
        
        bool dotDark = UiTheme::is_dark_theme();
        p.setBrush(m_selected ? QColor("#1a7eff") : (dotDark ? QColor("#555555") : QColor("#d0d0d0")));
        p.drawEllipse(r);
        
        int inner = r.width() - 6;
        int dotD = (m_width > 0) ? qMin(m_width * 2 + 2, inner) : 4;
        if (dotD < inner - 2) {
            QColor dotColor;
            if (m_toolName == "eraser") {
                dotColor = Qt::white;
            } else if (m_toolName == "mosaic") {
                dotColor = m_selected ? Qt::white : QColor("#1a7eff");
            } else if (!m_dotColor.isValid()) {
                dotColor = Qt::white;
            } else {
                dotColor = m_dotColor;
            }
            
            if (!m_selected && m_toolName != "mosaic" && m_toolName != "eraser") dotColor = dotColor.lighter(70);
            p.setBrush(dotColor);
            p.setPen(Qt::NoPen);
            qreal cx = (r.left() + r.right()) / 2.0;
            qreal cy = (r.top() + r.bottom()) / 2.0;
            p.drawEllipse(QPointF(cx, cy), dotD / 2.0, dotD / 2.0);
        } else {
            p.setPen(m_selected ? Qt::white : (dotDark ? QColor("#cccccc") : QColor("#666666")));
            QFont f = p.font();
            f.setBold(true);
            f.setPointSize(10);
            p.setFont(f);
            p.drawText(r, Qt::AlignCenter, QString::number(m_width));
        }
    }
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            emit clicked();
            e->accept();
        } else {
            QWidget::mousePressEvent(e);
        }
    }
signals:
    void wheelDelta(int delta);
    void clicked();
protected:
    void wheelEvent(QWheelEvent* e) override {
        emit wheelDelta(e->angleDelta().y());
        e->accept();
    }
private:
    QString m_toolName;
    int m_width = 0;
    QColor m_dotColor;
    bool m_selected = true;
public:
    void setColor(const QColor& c) { m_dotColor = c; update(); }
};


class ToolbarBg : public QFrame {
    Q_OBJECT
public:
    ToolbarBg(QWidget* parent = nullptr) : QFrame(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    void setColors(const QColor& bg, const QColor& border, const QColor& shadow) {
        m_bg = bg; m_border = border; m_shadow = shadow; update();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRect r = rect().adjusted(4, 4, -4, -4);
        for (int i = 0; i < 4; ++i) {
            int a = m_shadow.alpha() / (i + 2);
            QRect sr = r.adjusted(-i, -i + 1, i, i + 1);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(m_shadow.red(), m_shadow.green(), m_shadow.blue(), a));
            p.drawRoundedRect(sr, 8 + i, 8 + i);
        }
        p.setPen(QPen(m_border, 1));
        p.setBrush(m_bg);
        p.drawRoundedRect(r, 6, 6);
    }
private:
    QColor m_bg = QColor(255, 255, 255, 255);
    QColor m_border = QColor(176, 176, 176);
    QColor m_shadow = QColor(0, 0, 0, 60);
};

// ==================== StickerToolbar ====================
class StickerToolbar : public QFrame {
    Q_OBJECT
public:
    StickerToolbar(QWidget* parent = nullptr) : QFrame(parent) {
        setFrameStyle(QFrame::NoFrame);
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_AlwaysShowToolTips);
        setAttribute(Qt::WA_Hover);
        setAttribute(Qt::WA_TranslucentBackground);

        auto* mainLay = new QVBoxLayout(this);
        mainLay->setContentsMargins(2, 2, 2, 2);
        mainLay->setSpacing(0);

        auto* btnRow = new QWidget();
        btnRow->setObjectName("btnRow");
        auto* lay = new QHBoxLayout(btnRow);
        lay->setContentsMargins(6, 3, 6, 3);
        lay->setSpacing(1);

        struct BtnDef { QString key; QString label; QString tipKey; bool isTool; bool sepAfter; };
        QVector<BtnDef> btns = {
            {"shape", QChar(0x25AD), "sticker.shape", true, false},
            {"arrow", QString::fromUtf8("\xE2\x86\x92"), "sticker.arrow", true, false},
            {"pen", QString::fromUtf8("\xE2\x9C\x8E"), "sticker.pen", true, false},
            {"highlighter", QChar(0x25AE), "sticker.highlighter", true, false},
            {"mosaic", QChar(0x25A6), "sticker.mosaic", true, false},
            {"text", QStringLiteral("T"), "sticker.text", true, false},
            {"eraser", QString::fromUtf8("\xE2\x8C\xAB"), "sticker.eraser", true, true},
            {"undo", QString::fromUtf8("\xE2\x86\xB6"), "sticker.undo", false, false},
            {"redo", QString::fromUtf8("\xE2\x86\xB7"), "sticker.redo", false, true},
            {"save", QStringLiteral("\xF0\x9F\x92\xBE"), "sticker.save", false, false},
            {"copy", QStringLiteral("\xF0\x9F\x93\x8B"), "sticker.copy", false, false},
            {"pin", QStringLiteral("\xF0\x9F\x93\x8C"), "sticker.pin", false, false},
        };
        static const QMap<QString, QString> svgMap = {
            {"shape", SHAPE_ICON_SVG}, {"rect", RECT_ICON_SVG}, {"ellipse", ELLIPSE_ICON_SVG},
            {"arrow", ARROW_ICON_SVG}, {"pen", PEN_ICON_SVG}, {"highlighter", HIGHLIGHTER_ICON_SVG},
            {"text", TEXT_ICON_SVG}, {"eraser", ERASER_ICON_SVG}, {"undo", UNDO_ICON_SVG},
            {"redo", REDO_ICON_SVG}, {"pin", PIN_ICON_SVG},
            {"save", SAVE_ICON_SVG}, {"copy", COPY_ICON_SVG}, {"color", COLOR_ICON_SVG},
            {"mosaic", MOSAIC_ICON_SVG}, {"exit", EXIT_ICON_SVG},
        };
        for (const auto& d : btns) {
            auto* b = new QToolButton();
            b->setFixedSize(28, 26);
            b->setAutoRaise(true);
            b->setCheckable(d.isTool);
            b->setAttribute(Qt::WA_Hover);
            b->setAttribute(Qt::WA_AlwaysShowToolTips);
            b->setToolTip(I18n::t(d.tipKey, d.label));
            b->setToolTipDuration(5000);
            if (svgMap.contains(d.key)) {
                QIcon icon = renderSvgIcon(svgMap.value(d.key), 20);
                if (!icon.isNull()) { b->setIcon(icon); b->setIconSize(QSize(16, 16)); }
                else b->setText(d.label);
            } else {
                b->setText(d.label);
            }
            if (d.isTool)
                connect(b, &QToolButton::clicked, this, [this, d](){ emit toolChanged(d.key); });
            else
                connect(b, &QToolButton::clicked, this, [this, d](){ emit actionTriggered(d.key); });
            lay->addWidget(b);
            m_buttons[d.key] = b;
            m_btnTipKeys[d.key] = d.tipKey;
            m_btnLabels[d.key] = d.label;
            if (d.key == "pin") b->setIconSize(QSize(24, 24));
            if (d.sepAfter) {
                auto* sep = new QFrame();
                sep->setFrameShape(QFrame::VLine);
                sep->setFixedWidth(1);
                sep->setStyleSheet("color: #555;");
                lay->addWidget(sep);
            }
        }
        lay->addStretch(1);  // left-align buttons
        mainLay->addWidget(btnRow);
        applyStyle();

        
        adjustSize();
        int btnW = btnRow->sizeHint().width();
        setFixedWidth(btnW + 8);

        
        m_propWin = new ToolbarBg();
        m_propWin->setColors(m_bgColor, m_borderColor, m_shadowColor);
        auto* propWinLay = new QVBoxLayout(m_propWin);
        propWinLay->setContentsMargins(2, 2, 2, 2);
        propWinLay->setSpacing(0);

        m_contentRow = new QWidget(m_propWin);
        auto* contentLay = new QHBoxLayout(m_contentRow);
        contentLay->setContentsMargins(6, 2, 6, 3);
        contentLay->setSpacing(4);
        m_widthLabel = new WidthDotPreview(m_contentRow);
        connect(m_widthLabel, &WidthDotPreview::wheelDelta, this, &StickerToolbar::wheelOnDot);
        connect(m_widthLabel, &WidthDotPreview::clicked, this, [this](){ emit dotClicked(); });
        contentLay->addWidget(m_widthLabel);
        m_propHost = new QWidget(m_contentRow);
        m_propHost->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        auto* propLay = new QHBoxLayout(m_propHost);
        propLay->setContentsMargins(0, 0, 0, 0);
        propLay->setSpacing(2);
        contentLay->addWidget(m_propHost);
        m_colorHost = new QWidget(m_contentRow);
        m_colorHost->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        auto* colorLay = new QHBoxLayout(m_colorHost);
        colorLay->setContentsMargins(0, 0, 0, 0);
        colorLay->setSpacing(2);
        contentLay->addWidget(m_colorHost);
        m_contentRow->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        m_contentRow->setVisible(true);
        propWinLay->addWidget(m_contentRow);
        m_propWin->adjustSize();
        m_propWin->hide();
    }

    void applyStyle() {
        refreshIcons();
        bool dark = UiTheme::is_dark_theme();
        QString bg = dark ? "#2d2d2d" : "#ffffff";
        QString border = dark ? "#4a4a4a" : "#b0b0b0";
        QString hover = dark ? "#404040" : "#e8e8e8";
        QString text = dark ? "#ffffff" : "#333333";
        QString checkedBg = "#1a7eff";
        
        m_bgColor = dark ? QColor(45, 45, 45, 255) : QColor(255, 255, 255, 255);
        m_borderColor = dark ? QColor(74, 74, 74) : QColor(176, 176, 176);
        m_shadowColor = dark ? QColor(0, 0, 0, 120) : QColor(0, 0, 0, 60);
        if (m_propWin) m_propWin->setColors(m_bgColor, m_borderColor, m_shadowColor);
        
        setStyleSheet(QString(
            "QToolButton { background: transparent; border: none; border-radius: 3px; color: %1; font-size: 12px; padding: 1px; }"
            "QToolButton:hover { background: %2; }"
            "QToolButton:checked { background: %3; color: white; }"
            "QToolButton:disabled { color: #555555; background: transparent; }"
            "QToolTip { background: %4; color: %1; border: 1px solid %5; border-radius: 3px; padding: 3px 6px; font-size: 11px; }"
).arg(text, hover, checkedBg, bg, border));

    }

    void refreshIcons() {
        static const QMap<QString, QString> svgMap = {
            {"shape", SHAPE_ICON_SVG}, {"rect", RECT_ICON_SVG}, {"ellipse", ELLIPSE_ICON_SVG},
            {"arrow", ARROW_ICON_SVG}, {"pen", PEN_ICON_SVG}, {"highlighter", HIGHLIGHTER_ICON_SVG},
            {"text", TEXT_ICON_SVG}, {"eraser", ERASER_ICON_SVG}, {"undo", UNDO_ICON_SVG},
            {"redo", REDO_ICON_SVG}, {"pin", PIN_ICON_SVG},
            {"save", SAVE_ICON_SVG}, {"copy", COPY_ICON_SVG}, {"color", COLOR_ICON_SVG},
            {"mosaic", MOSAIC_ICON_SVG}, {"exit", EXIT_ICON_SVG},
        };
        for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
            if (svgMap.contains(it.key())) {
                QIcon icon = renderSvgIcon(svgMap.value(it.key()), 20);
                if (!icon.isNull()) it.value()->setIcon(icon);
            }
        }
    }

    
    void setActiveTool(const QString& key) {
        for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
            auto* btn = it.value();
            if (!btn->isCheckable()) continue;
            QString bk = it.key();
            bool on = (bk == key) || (bk == "shape" && (key == "rect" || key == "ellipse"));
            btn->setChecked(on);
        }
    }

    void updateDotColor(const QColor& c) {
        if (m_widthLabel) m_widthLabel->setColor(c);
    }
    void setDotSelected(bool sel) {
        if (m_widthLabel) m_widthLabel->setSelected(sel);
    }
    void updateWidthLabel(const QString& toolName, int width) {
        if (!toolName.isEmpty() && width > 0) {
            m_widthLabel->setInfo(toolName, width);
            m_widthLabel->setVisible(true);
        } else {
            m_widthLabel->setVisible(false);
        }
        adjustSize();
    }
    void refreshLanguage() {
        for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
            QString key = it.key();
            if (m_btnTipKeys.contains(key))
                it.value()->setToolTip(I18n::t(m_btnTipKeys[key], m_btnLabels[key]));
        }
    }

    void updateUndoRedo(bool canUndo, bool canRedo) {
        if (m_buttons.contains("undo")) m_buttons["undo"]->setEnabled(canUndo);
        if (m_buttons.contains("redo")) m_buttons["redo"]->setEnabled(canRedo);
    }

    void reposition(const QRect& stickerGeo, const QRect& screenGeo) {
        
        int tw = width(), th = sizeHint().height();
        int gap = 4;
        int x = stickerGeo.right() - tw;
        int y = stickerGeo.bottom() + gap;
        if (y + th > screenGeo.bottom()) y = stickerGeo.top() - th - gap;
        if (x < screenGeo.left()) x = screenGeo.left();
        move(x, y);
        if (m_propWin && m_propWin->isVisible()) positionPropWin();
    }

    void positionPanelBelow(QWidget* panel, const QString& anchorBtn, int gap = 6) {
        QToolButton* btn = m_buttons.value(anchorBtn, nullptr);
        if (!btn || !panel) return;
        QApplication::processEvents();
        panel->adjustSize();
        // Default: below the button
        QPoint gp = btn->mapToGlobal(QPoint(0, btn->height() + gap));
        int panelH = panel->height();
        QScreen* screen = QApplication::screenAt(gp);
        if (!screen) screen = QApplication::primaryScreen();
        QRect screenGeo = screen ? screen->availableGeometry() : QRect();
        // If panel would go off bottom of screen, flip above the toolbar
        if (screenGeo.isValid() && gp.y() + panelH > screenGeo.bottom()) {
            // Place above the toolbar instead
            gp = btn->mapToGlobal(QPoint(0, -gap));
            gp.ry() -= panelH;
        }
        // Clamp X within screen
        if (screenGeo.isValid()) {
            if (gp.x() + panel->width() > screenGeo.right())
                gp.setX(screenGeo.right() - panel->width());
            if (gp.x() < screenGeo.left())
                gp.setX(screenGeo.left());
        }
        panel->move(gp);
    }

    QToolButton* button(const QString& key) { return m_buttons.value(key, nullptr); }

    // ===== Unified panel control =====
    QWidget* propHost() { return m_propHost; }
    QWidget* colorHost() { return m_colorHost; }

    void setPropPanel(QWidget* panel) {
        if (!m_propHost) return;
        // Hide all current children of m_propHost (only direct widget children, skip layout)
        const auto kids = m_propHost->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget* w : kids) w->hide();
        if (panel) {
            panel->setParent(m_propHost);
            panel->show();
            panel->adjustSize();
            panel->updateGeometry();
            m_propHost->setVisible(true);
            m_propVisible = true;
            QTimer::singleShot(0, this, [this](){ refreshLayout(); });
        } else {
            m_propHost->setVisible(false);
            m_propVisible = false;
        }
        refreshLayout();
    }

    void setColorPanelVisible(bool visible) {
        if (m_colorHost) m_colorHost->setVisible(visible);
        m_colorVisible = visible;
        refreshLayout();
    }

    void showUnifiedPanel() {
        bool showContent = m_propVisible || m_colorVisible;
        if (m_propWin) {
            m_propWin->setVisible(showContent);
            if (showContent) {
                refreshLayout();
                positionPropWin();
                m_propWin->raise();
            }
        }
        show();
        raise();
        
        QTimer::singleShot(0, this, [this]() {
            refreshLayout();
            positionPropWin();
        });
    }

    void positionPropWin() {
        if (!m_propWin) return;
        int gap = 1;
        int px = this->x();
        int py = this->y() + height() + gap;
        m_propWin->move(px, py);
    }

    void hideUnifiedPanel() {
        if (m_propWin) m_propWin->hide();
        hide();
    }

    void refreshLayout() {
        adjustSize();
        if (m_propWin && m_contentRow) {
            
            m_contentRow->adjustSize();
            if (m_propHost) m_propHost->adjustSize();
            if (m_colorHost) m_colorHost->adjustSize();
            m_propWin->adjustSize();
            int w = m_propWin->sizeHint().width();
            if (w > 0) m_propWin->setFixedWidth(w);
        }
    }

protected:
    void paintEvent(QPaintEvent*) override;

signals:
    void toolChanged(const QString& key);
    void actionTriggered(const QString& key);
    void wheelOnDot(int delta);
    void dotClicked();

private:
    QMap<QString, QToolButton*> m_buttons;
    QMap<QString, QString> m_btnTipKeys;
    QMap<QString, QString> m_btnLabels;
    WidthDotPreview* m_widthLabel = nullptr;
    ToolbarBg* m_propWin = nullptr;
    QWidget* m_contentRow = nullptr;
    QWidget* m_propHost = nullptr;
    QWidget* m_colorHost = nullptr;
    bool m_propVisible = false;
    bool m_colorVisible = false;
    
    QColor m_bgColor = QColor(255, 255, 255, 255);
    QColor m_borderColor = QColor(176, 176, 176);
    QColor m_shadowColor = QColor(0, 0, 0, 60);
};

void StickerToolbar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    
    QRect r = rect().adjusted(4, 4, -4, -4);
    for (int i = 0; i < 4; ++i) {
        int a = m_shadowColor.alpha() / (i + 2);
        QRect sr = r.adjusted(-i, -i + 1, i, i + 1);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(m_shadowColor.red(), m_shadowColor.green(), m_shadowColor.blue(), a));
        p.drawRoundedRect(sr, 8 + i, 8 + i);
    }

    
    p.setPen(QPen(m_borderColor, 1));
    p.setBrush(m_bgColor);
    p.drawRoundedRect(r, 6, 6);
}

// ==================== StickerWindow ====================
StickerWindow::StickerWindow(const QPixmap& pixmap, QWidget* parent,
                             const QPoint& pos,
                             const QString& historyFilePath,
                             const QString& originalFilePath,
                             qreal scale)
    : QWidget(parent), m_pixmap(pixmap), m_originalPixmap(pixmap),
      m_historyFilePath(historyFilePath), m_originalFilePath(originalFilePath), m_scale(scale) {
    
    qreal dpr = pixmap.devicePixelRatio() > 0 ? pixmap.devicePixelRatio() : 1.0;
    m_imgW = int(pixmap.width() / dpr);
    m_imgH = int(pixmap.height() / dpr);
    m_origImgW = m_imgW;
    m_origImgH = m_imgH;
    m_pixmap = pixmap;
    m_pixmap.setDevicePixelRatio(dpr);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_AlwaysShowToolTips);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);

    int m = SHADOW_MARGIN;
    setFixedSize(m_imgW + m * 2, m_imgH + m * 2);
    if (!pos.isNull()) move(pos);

    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, this, [this](){ m_toastText.clear(); update(); });

    m_showToolbar = false;  
    buildToolbar();
    applyWin11DwmAttributes();
    // Load magnifier config
    QJsonObject magCfg = qvariant_cast<QJsonObject>(ConfigManager::instance().get("magnifier"));
    m_magVisible = magCfg.value("enabled").toBool(true);
    m_colorFmt = magCfg.value("color_format").toString("RGB");
    m_hideHash = magCfg.value("hide_hash").toBool(false);
    m_hexUpper = magCfg.value("hex_uppercase").toBool(false);
}

void StickerWindow::applyWin11DwmAttributes() {
#ifdef Q_OS_WIN
    HWND hwnd = (HWND)winId();
    if (!hwnd) return;
    
    BOOL dark = UiTheme::is_dark_theme() ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
    DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
    // Win11: disable rounded corners to avoid transparent edge (DWMWA_WINDOW_CORNER_PREFERENCE=33)
    int cornerPref = 1;  // DWMWCP_DONOTROUND - disable rounded corners (no transparent border)
    DwmSetWindowAttribute(hwnd, 33, &cornerPref, sizeof(cornerPref));
    
    BOOL disableTrans = TRUE;
    DwmSetWindowAttribute(hwnd, 3, &disableTrans, sizeof(disableTrans));
#endif
}

StickerWindow::~StickerWindow() {
    if (m_textEditor) {
        m_textEditor->deleteLater();
        m_textEditor = nullptr;
    }
    // All panels (color/font/shape/arrow/mosaic/hl/eraser) are children of m_toolbar;
    // Qt cascades deletion automatically.
    if (m_toolbar) {
        m_toolbar->close();
        delete m_toolbar;
        m_toolbar = nullptr;
    }
}

void StickerWindow::buildToolbar() {
    m_toolbar = new StickerToolbar();
    m_toolbar->hide();
    connect(m_toolbar, &StickerToolbar::toolChanged, this, &StickerWindow::onToolChanged);
    connect(m_toolbar, &StickerToolbar::actionTriggered, this, &StickerWindow::onAction);

    // Color panel: shared single instance, lives in m_colorHost (right segment of unified panel)
    m_colorPanel = new ColorPanel(m_toolbar->colorHost(), m_annColor);
    m_toolbar->updateDotColor(m_annColor);
    
    connect(m_toolbar, &StickerToolbar::wheelOnDot, this, [this](int delta){
        adjustToolWidth(delta);
    });
    
    connect(m_toolbar, &StickerToolbar::dotClicked, this, [this](){
        if (m_currentTool == Tool::Eraser) onEraserSelected("free");
        else if (m_currentTool == Tool::Highlighter) onHlSelected("sketch");
        else if (m_currentTool == Tool::Mosaic) onMosaicFill(false);
    });
    connect(m_colorPanel, &ColorPanel::colorPicked, this, &StickerWindow::onColorPicked);
    if (auto* lay = qobject_cast<QHBoxLayout*>(m_toolbar->colorHost()->layout()))
        lay->addWidget(m_colorPanel);

    // Property panels: one per tool, embedded in m_propHost (left segment)
    auto addProp = [this](QWidget* p) {
        if (auto* lay = qobject_cast<QHBoxLayout*>(m_toolbar->propHost()->layout()))
            lay->addWidget(p);
        p->hide();
    };

    m_fontPanel = new FontPanel(m_toolbar->propHost(), m_annFont);
    connect(m_fontPanel, &FontPanel::fontChanged, this, [this](const QFont& f){
        m_annFont = f;
        if (m_textEditor) {
            m_textEditor->setFont(m_annFont);
            QFontMetrics fm(m_annFont);
            int w = qMax(120, fm.horizontalAdvance(m_textEditor->text()) + 20);
            int h = fm.height() + 8;
            m_textEditor->resize(w, h);
        }
    });
    addProp(m_fontPanel);

    m_penPanel = new PenPanel(m_toolbar->propHost(), m_penWidth);
    connect(m_penPanel, &PenPanel::widthChanged, this, [this](int w){ m_penWidth = w; syncWidthLabel(); });
    addProp(m_penPanel);

    m_shapePanel = new ShapeTypePanel(m_toolbar->propHost(), m_shapeTool, m_shapeFill);
    connect(m_shapePanel, &ShapeTypePanel::shapeSelected, this, &StickerWindow::onShapeSelected);
    addProp(m_shapePanel);

    m_arrowPanel = new ArrowTypePanel(m_toolbar->propHost(), m_arrowStyle);
    connect(m_arrowPanel, &ArrowTypePanel::arrowSelected, this, &StickerWindow::onArrowSelected);
    addProp(m_arrowPanel);

    m_mosaicPanel = new MosaicPanel(m_toolbar->propHost(), m_mosaicBlock, m_mosaicMode, m_mosaicFill, m_mosaicOpacity);
    connect(m_mosaicPanel, &MosaicPanel::modeChanged, this, [this](const QString& m){ m_mosaicMode = m; });
    connect(m_mosaicPanel, &MosaicPanel::fillChanged, this, [this](bool f){ onMosaicFill(f); });
    connect(m_mosaicPanel, &MosaicPanel::opacityChanged, this, [this](int op){ m_mosaicOpacity = op; update(); });
    addProp(m_mosaicPanel);

    m_hlPanel = new HighlighterTypePanel(m_toolbar->propHost(), m_hlMode);
    connect(m_hlPanel, &HighlighterTypePanel::hlSelected, this, &StickerWindow::onHlSelected);
    addProp(m_hlPanel);

    m_eraserPanel = new EraserTypePanel(m_toolbar->propHost(), m_eraserMode);
    connect(m_eraserPanel, &EraserTypePanel::eraserSelected, this, &StickerWindow::onEraserSelected);
    addProp(m_eraserPanel);
}

QPoint StickerWindow::toImagePos(const QPoint& windowPos) const {
    return QPoint(windowPos.x() - SHADOW_MARGIN, windowPos.y() - SHADOW_MARGIN);
}
QPoint StickerWindow::toWindowPos(const QPoint& imagePos) const {
    return QPoint(imagePos.x() + SHADOW_MARGIN, imagePos.y() + SHADOW_MARGIN);
}

void StickerWindow::setSelected(bool selected) {
    if (m_selected == selected && selected) return;
    if (selected) {
        for (QWidget* w : QApplication::topLevelWidgets()) {
            auto* s = qobject_cast<StickerWindow*>(w);
            if (s && s != this && s->isSelected()) {
                s->m_selected = false;
                s->updateToolbar();
                s->update();
            }
        }
    }
    m_selected = selected;
    updateToolbar();
    update();
}

void StickerWindow::updateToolbar() {
    if (!m_toolbar) return;
    if (m_selected && m_showToolbar) {
        repositionToolbar();
        m_toolbar->show();
        m_toolbar->raise();
    } else {
        m_toolbar->hide();
        closeAllPanels();
        if (m_currentTool != Tool::None) {
            m_currentTool = Tool::None;
            m_toolbar->setActiveTool(QString());
            setCursor(Qt::ArrowCursor);
        }
    }
    refreshUndoRedo();
}

void StickerWindow::repositionToolbar() {
    if (!m_toolbar) return;
    QScreen* screen = QApplication::screenAt(geometry().center());
    if (!screen) screen = QApplication::primaryScreen();
    if (screen) m_toolbar->reposition(geometry(), screen->availableGeometry());
    // Reposition any open sub-panels
    repositionSubPanels();
}

void StickerWindow::repositionSubPanels() {
    // Panels are now embedded children of m_toolbar; no separate floating sub-panels to reposition.
}

void StickerWindow::closeAllPanels() {
    hideUnifiedPanel();
}

void StickerWindow::applyTheme() {
    if (m_toolbar) m_toolbar->applyStyle();
    if (m_fontPanel) m_fontPanel->applyStyle();
    if (m_penPanel) m_penPanel->applyStyle();
    if (m_shapePanel) m_shapePanel->applyStyle();
    if (m_arrowPanel) m_arrowPanel->applyStyle();
    if (m_mosaicPanel) m_mosaicPanel->applyStyle();
    if (m_hlPanel) m_hlPanel->applyStyle();
    if (m_eraserPanel) m_eraserPanel->applyStyle();
    applyWin11DwmAttributes();
}

void StickerWindow::refreshLanguage() {
    if (m_toolbar) m_toolbar->refreshLanguage();
    if (m_fontPanel) m_fontPanel->refreshLanguage();
    if (m_shapePanel) m_shapePanel->refreshLanguage();
    if (m_arrowPanel) m_arrowPanel->refreshLanguage();
    if (m_mosaicPanel) m_mosaicPanel->refreshLanguage();
    if (m_hlPanel) m_hlPanel->refreshLanguage();
    if (m_eraserPanel) m_eraserPanel->refreshLanguage();
    if (m_colorPanel) m_colorPanel->refreshLanguage();
    syncWidthLabel();
}

void StickerWindow::refreshUndoRedo() {
    if (m_toolbar) m_toolbar->updateUndoRedo(!m_annotations.isEmpty(), !m_redoStack.isEmpty());
}

void StickerWindow::setTopmost(bool topmost) {
#ifdef Q_OS_WIN
    if (s_topmostSuspended) return;  
    HWND hwnd = (HWND)winId();
    if (!hwnd) return;
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
    Q_UNUSED(topmost)
#endif
}

void StickerWindow::enterEvent(QEnterEvent* e) {
    QWidget::enterEvent(e);
    setTopmost(true);
}
void StickerWindow::leaveEvent(QEvent* e) {
    QWidget::leaveEvent(e);
    
    
}

void StickerWindow::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);
    // Snipaste-style z-order: do NOT force topmost on ActivationChange.
    // When user clicks another window, that window should come forward.
    // Only re-assert topmost on WindowStateChange (minimize/restore).
    if (e->type() == QEvent::WindowStateChange) {
        setTopmost(true);
    }
}

void StickerWindow::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    setTopmost(true);
    
}


void StickerWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setRenderHint(QPainter::Antialiasing, true);

    int m = SHADOW_MARGIN;
    QRect imgRect(m, m, m_imgW, m_imgH);

    
    QColor border;
    if (m_shadowEnabled) {
        QColor shadowColor;
        int alphaBase;
        int shadowRange;
        if (m_selected) {
            shadowColor = QColor(26, 174, 255);
            alphaBase = 50;
            border = QColor(26, 174, 255, 240);
            shadowRange = (std::max)(1, m / 3);
        } else {
            shadowColor = QColor(120, 120, 120);
            alphaBase = 25;
            border = QColor(100, 100, 100, 140);
            shadowRange = m;
        }
        qreal r = 3;
        for (int i = 0; i < shadowRange; i++) {
            int alpha = int(alphaBase * (1 - (qreal)i / (std::max)(1, shadowRange)));
            int expand = i;
            QRectF sr = QRectF(imgRect).adjusted(-expand, -expand + 2, expand, expand + 2);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(shadowColor.red(), shadowColor.green(), shadowColor.blue(), alpha));
            p.drawRoundedRect(sr, r + expand * 0.5, r + expand * 0.5);
        }
    } else {
        border = QColor(100, 100, 100, 80);
    }

    
    if (m_cacheDirty) rebuildStaticCache();
    p.drawPixmap(rect(), m_staticCache);

    
    if (m_drawing) drawCurrentAnnotation(p);

    
    if (m_currentTool == Tool::None && !m_annotations.isEmpty()) {
        for (const auto& ann : m_annotations) {
            if (ann.tool == Tool::Arrow) {
                QPoint wp = toWindowPos(ann.start);
                int r = 5;
                p.setPen(QPen(Qt::white, 2));
                p.setBrush(QColor(26, 174, 255));
                p.drawEllipse(wp, r, r);
            }
        }
    }

    
    int activeTextIdx = m_draggingText ? m_draggingTextIdx : (m_resizingText ? m_resizingTextIdx : -1);
    if (activeTextIdx >= 0 && activeTextIdx < m_annotations.size()) {
        Annotation ann = m_annotations[activeTextIdx];
        ann.selected = true;
        drawAnnotation(p, ann);
    }

    
    p.setPen(border);
    p.setBrush(Qt::NoBrush);
    p.drawRect(imgRect.adjusted(0, 0, -1, -1));

    
    if (!m_toastText.isEmpty()) {
        QFont f("Microsoft YaHei", 10);
        p.setFont(f);
        QFontMetrics fm(f);
        int tw = fm.horizontalAdvance(m_toastText);
        int th = fm.ascent();
        int padX = 16, padY = 8;
        qreal boxW = tw + padX * 2;
        qreal boxH = th + padY * 2;
        qreal bx = imgRect.center().x() - boxW / 2.0;
        qreal by = imgRect.center().y() - boxH / 2.0;
        QRectF box(bx, by, boxW, boxH);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 180));
        p.drawRoundedRect(box, 8, 8);
        p.setPen(QColor(255, 255, 255));
        p.drawText(box, Qt::AlignCenter, m_toastText);
    }

    
    // drawMagnifier(p);
}

void StickerWindow::markCacheDirty() { m_cacheDirty = true; }

void StickerWindow::rebuildStaticCache() {
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    QSize physSize(int(size().width() * dpr), int(size().height() * dpr));
    if (m_staticCache.size() != physSize || m_staticCache.devicePixelRatio() != dpr) {
        m_staticCache = QPixmap(physSize);
        m_staticCache.setDevicePixelRatio(dpr);
    }
    m_staticCache.fill(Qt::transparent);
    QPainter p(&m_staticCache);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPixmap(QRect(SHADOW_MARGIN, SHADOW_MARGIN, m_imgW, m_imgH), m_pixmap);
    for (int i = 0; i < m_annotations.size(); i++) {
        m_annotations[i].selected = false;
        drawAnnotation(p, m_annotations[i]);
    }
    p.end();
    m_cacheDirty = false;
}


void StickerWindow::drawAnnotation(QPainter& p, const Annotation& ann) {
    switch (ann.tool) {
    case Tool::Rect:
    case Tool::Ellipse: {
        QPoint s = toWindowPos(ann.start);
        QPoint e = toWindowPos(ann.end);
        QRect r = QRect(s, e).normalized();
        p.setPen(QPen(ann.color, ann.width));
        
        if (ann.fill)
            p.setBrush(QBrush(ann.color));
        else
            p.setBrush(Qt::NoBrush);
        if (ann.tool == Tool::Rect) p.drawRect(r);
        else p.drawEllipse(r);
        break;
    }
    case Tool::Arrow:
        drawArrow(p, toWindowPos(ann.start), toWindowPos(ann.end), ann.color, ann.width, ann.arrowStyle);
        break;
    case Tool::Highlighter:
        if (ann.hlMode == "box") {
            QRect r = QRect(toWindowPos(ann.start), toWindowPos(ann.end)).normalized();
            QColor fc(ann.color.red(), ann.color.green(), ann.color.blue(), 60);
            p.setPen(QPen(fc, 1));
            p.setBrush(fc);
            p.drawRect(r);
        } else {
            if (ann.points.size() < 2) return;
            QPen pen(QColor(ann.color.red(), ann.color.green(), ann.color.blue(), 20), ann.width);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            p.setPen(pen);
            QPainterPath hlPath;
            hlPath.moveTo(toWindowPos(ann.points[0]));
            for (int i = 1; i < ann.points.size(); i++)
                hlPath.lineTo(toWindowPos(ann.points[i]));
            p.drawPath(hlPath);
        }
        break;
    case Tool::Pen: {
        if (ann.points.size() < 2) return;
        p.setPen(QPen(ann.color, ann.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath penPath;
        penPath.moveTo(toWindowPos(ann.points[0]));
        for (int i = 1; i < ann.points.size(); i++)
            penPath.lineTo(toWindowPos(ann.points[i]));
        p.drawPath(penPath);
        break;
    }
    case Tool::Mosaic: {
        if (ann.mosaicFill) {
            QRect r = QRect(toWindowPos(ann.start), toWindowPos(ann.end)).normalized();
            if (ann.mosaicMode == "blur") drawBlur(p, r, ann.blockSize, ann.mosaicOpacity);
            else drawMosaic(p, r, ann.blockSize, ann.mosaicOpacity);
        } else {
            int radius = ann.width > 0 ? ann.width : ann.blockSize;
            for (const QPoint& pt : ann.points) {
                QPoint wp = toWindowPos(pt);
                int rr = radius;
                QRect rect(wp.x() - rr, wp.y() - rr, rr * 2, rr * 2);
                if (ann.mosaicMode == "blur") drawBlur(p, rect, ann.blockSize, ann.mosaicOpacity);
                else drawMosaic(p, rect, ann.blockSize, ann.mosaicOpacity);
            }
        }
        break;
    }
    case Tool::Text: {
        QPoint pos = toWindowPos(ann.pos);
        p.setFont(ann.font);
        p.setPen(ann.color);
        p.drawText(pos, ann.text);
        if (ann.selected) {
            QFontMetrics fm(ann.font);
            int tw = fm.horizontalAdvance(ann.text);
            int th = fm.ascent();
            QRect box(pos.x() - 2, pos.y() - th - 2, tw + 4, th + fm.descent() + 4);
            p.setPen(QPen(QColor(26, 174, 255, 200), 1, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawRect(box);
            int hs = 8;
            QRect handle(box.right() - hs / 2, box.bottom() - hs / 2, hs, hs);
            p.setPen(QPen(QColor(255, 255, 255), 1));
            p.setBrush(QColor(26, 174, 255));
            p.drawRect(handle);
        }
        break;
    }
    case Tool::Eraser:
        break;
    default: break;
    }
}


void StickerWindow::drawCurrentAnnotation(QPainter& p) {
    if (m_currentTool == Tool::None) return;
    Annotation ann;
    ann.tool = m_currentTool;
    ann.color = m_annColor;
    ann.arrowStyle = m_arrowStyle;
    ann.hlMode = m_hlMode;
    ann.blockSize = m_mosaicBlock;
    ann.mosaicOpacity = m_mosaicOpacity;
    ann.mosaicMode = m_mosaicMode;
    ann.mosaicFill = m_mosaicFill;

    switch (m_currentTool) {
    case Tool::Rect:
    case Tool::Ellipse:
        ann.start = m_drawStart; ann.end = m_drawCurrent;
        ann.width = m_shapeWidth;
        ann.fill = m_shapeFill;  
        drawAnnotation(p, ann);
        break;
    case Tool::Arrow:
        ann.start = m_drawStart; ann.end = m_drawCurrent;
        ann.width = m_arrowWidth;
        ann.fill = false;
        drawAnnotation(p, ann);
        break;
    case Tool::Highlighter:
        if (m_hlMode == "box") {
            ann.start = m_drawStart; ann.end = m_drawCurrent;
            ann.width = m_highlighterWidth;
            drawAnnotation(p, ann);
        } else {
            ann.points = m_freehandPoints;
            ann.width = m_highlighterWidth;
            drawAnnotation(p, ann);
        }
        break;
    case Tool::Pen:
        ann.points = m_freehandPoints;
        ann.width = m_penWidth;
        drawAnnotation(p, ann);
        break;
    case Tool::Mosaic:
        if (m_mosaicFill) {
            ann.start = m_drawStart; ann.end = m_drawCurrent;
            ann.width = m_mosaicBlock;
            drawAnnotation(p, ann);
        } else {
            ann.points = m_freehandPoints;
            ann.width = m_mosaicBlock;
            drawAnnotation(p, ann);
        }
        break;
    case Tool::Eraser:
        if (m_eraserMode == "fill") {
            QRect r = QRect(toWindowPos(m_drawStart), toWindowPos(m_drawCurrent)).normalized();
            p.setPen(QPen(QColor(255, 255, 255, 180), 1, Qt::DashLine));
            p.setBrush(QColor(255, 255, 255, 30));
            p.drawRect(r);
        } else {
            QPoint c = toWindowPos(m_drawCurrent);
            p.setPen(QPen(QColor(255, 255, 255, 180), 1, Qt::DashLine));
            p.setBrush(QColor(255, 255, 255, 30));
            p.drawEllipse(c, m_eraserWidth, m_eraserWidth);
        }
        break;
    default: break;
    }
}

void StickerWindow::drawArrow(QPainter& p, const QPoint& start, const QPoint& end,
                              const QColor& color, int width, ArrowStyle style) {
    drawArrowShape(p, QPointF(start), QPointF(end), color, width, style);
}

void StickerWindow::drawMosaic(QPainter& p, const QRect& rect, int blockSize, int opacity) {
    if (rect.width() <= 0 || rect.height() <= 0 || blockSize < 1) return;
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    
    QRect imgRect(rect.x() - SHADOW_MARGIN, rect.y() - SHADOW_MARGIN, rect.width(), rect.height());
    QRect physRect(int(imgRect.x() * dpr), int(imgRect.y() * dpr),
                   int(imgRect.width() * dpr), int(imgRect.height() * dpr));
    QImage fullImg = m_pixmap.toImage();
    QRect pixBounds(0, 0, fullImg.width(), fullImg.height());
    QRect srcRect = physRect.intersected(pixBounds);
    if (srcRect.isEmpty()) return;
    QImage img = fullImg.copy(srcRect);
    int bs = (std::max)(1, blockSize);
    QImage mosaic(img.size(), QImage::Format_RGB32);
    for (int y = 0; y < img.height(); y += bs) {
        for (int x = 0; x < img.width(); x += bs) {
            int x2 = (std::min)(x + bs, img.width());
            int y2 = (std::min)(y + bs, img.height());
            long r = 0, g = 0, b = 0, cnt = 0;
            for (int yy = y; yy < y2; yy++)
                for (int xx = x; xx < x2; xx++) {
                    QRgb c = img.pixel(xx, yy);
                    r += qRed(c); g += qGreen(c); b += qBlue(c); cnt++;
                }
            if (cnt == 0) continue;
            QRgb avg = qRgb(int(r / cnt), int(g / cnt), int(b / cnt));
            for (int yy = y; yy < y2; yy++)
                for (int xx = x; xx < x2; xx++)
                    mosaic.setPixel(xx, yy, avg);
        }
    }
    
    mosaic.setDevicePixelRatio(dpr);
    
    qreal alpha = qBound(0.0, qreal(opacity) / 24.0, 1.0);
    p.save();
    p.setOpacity(alpha);
    p.drawImage(QRect(rect.topLeft(), imgRect.size()), mosaic);
    p.restore();
}

void StickerWindow::drawBlur(QPainter& p, const QRect& rect, int blockSize, int opacity) {
    if (rect.width() <= 0 || rect.height() <= 0) return;
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    QRect imgRect(rect.x() - SHADOW_MARGIN, rect.y() - SHADOW_MARGIN, rect.width(), rect.height());
    QRect physRect(int(imgRect.x() * dpr), int(imgRect.y() * dpr),
                   int(imgRect.width() * dpr), int(imgRect.height() * dpr));
    QImage fullImg = m_pixmap.toImage();
    QRect pixBounds(0, 0, fullImg.width(), fullImg.height());
    QRect srcRect = physRect.intersected(pixBounds);
    if (srcRect.isEmpty()) return;
    QImage srcImg = fullImg.copy(srcRect);
    int factor = (std::max)(1, blockSize / 2);
    QImage smallImg = srcImg.scaled((std::max)(1, srcImg.width() / factor),
                                    (std::max)(1, srcImg.height() / factor),
                                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage blurredImg = smallImg.scaled(srcImg.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    blurredImg.setDevicePixelRatio(dpr);
    qreal alpha = qBound(0.0, qreal(opacity) / 24.0, 1.0);
    p.save();
    p.setOpacity(alpha);
    p.drawImage(QRect(rect.topLeft(), imgRect.size()), blurredImg);
    p.restore();
}


void StickerWindow::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return;
    QPoint imgPos = toImagePos(e->pos());

    if (m_currentTool == Tool::Eraser) {
        m_drawing = true;
        m_drawStart = imgPos;
        m_drawCurrent = imgPos;
        if (m_eraserMode == "free") {
            m_freehandPoints.clear();
            m_freehandPoints.append(imgPos);
            m_lastErasePos = imgPos;
            eraseAtPos(imgPos);
        }
        return;
    }

    if (m_currentTool == Tool::Text) {
        if (m_textEditor) finishInlineEdit();
        // Check if clicking on text resize handle → start resizing
        for (int i = m_annotations.size() - 1; i >= 0; --i) {
            if (m_annotations[i].tool == Tool::Text && hitTextHandle(m_annotations[i], imgPos)) {
                m_resizingText = true;
                m_resizingTextIdx = i;
                m_resizeStartSize = m_annotations[i].font.pointSizeF();
                if (m_resizeStartSize <= 0) m_resizeStartSize = 16;
                m_resizeStartDist = (imgPos - m_annotations[i].pos).manhattanLength();
                setSelected(true);
                setCursor(Qt::SizeFDiagCursor);
                return;
            }
            if (m_annotations[i].tool == Tool::Text && hitTest(m_annotations[i], imgPos)) {
                m_draggingText = true;
                m_draggingTextIdx = i;
                m_textDragOffset = imgPos - m_annotations[i].pos;
                setSelected(true);
                setCursor(Qt::SizeAllCursor);
                return;
            }
        }
        // Click on empty space: create new text
        setSelected(true);
        startInlineEdit(toImagePos(e->pos()));
        return;
    }

    if (m_currentTool != Tool::None) {
        m_drawing = true;
        m_drawStart = imgPos;
        m_drawCurrent = imgPos;
        m_freehandPoints.clear();
        m_freehandPoints.append(imgPos);
        return;
    }

    
    for (int i = m_annotations.size() - 1; i >= 0; --i) {
        if (m_annotations[i].tool == Tool::Text && hitTextHandle(m_annotations[i], imgPos)) {
            m_resizingText = true;
            m_resizingTextIdx = i;
            m_resizeStartSize = m_annotations[i].font.pointSizeF();
            if (m_resizeStartSize <= 0) m_resizeStartSize = 16;
            m_resizeStartDist = (imgPos - m_annotations[i].pos).manhattanLength();
            setSelected(true);
            setCursor(Qt::SizeFDiagCursor);
            return;
        }
        if (m_annotations[i].tool == Tool::Text && hitTest(m_annotations[i], imgPos)) {
            m_draggingText = true;
            m_draggingTextIdx = i;
            m_textDragOffset = imgPos - m_annotations[i].pos;
            setSelected(true);
            setCursor(Qt::SizeAllCursor);
            return;
        }
    }

    
    for (int i = m_annotations.size() - 1; i >= 0; --i) {
        if (m_annotations[i].tool == Tool::Arrow) {
            QString ep = hitArrowEndpoint(m_annotations[i], imgPos);
            if (!ep.isEmpty()) {
                m_draggingArrow = true;
                m_draggingArrowIdx = i;
                m_draggingArrowEndpoint = ep;
                setSelected(true);
                setCursor(Qt::SizeAllCursor);
                return;
            }
        }
    }

    
    setSelected(true);
    m_dragging = true;
    m_dragOffset = e->globalPos() - frameGeometry().topLeft();
    setCursor(Qt::SizeAllCursor);
}

void StickerWindow::mouseMoveEvent(QMouseEvent* e) {
    m_mousePos = e->pos();  
    QPoint imgPos = toImagePos(e->pos());

    if (m_dragging) {
        move(e->globalPos() - m_dragOffset);
        repositionToolbar();
        return;
    }

    if (m_draggingText && m_draggingTextIdx >= 0 && m_draggingTextIdx < m_annotations.size()) {
        m_annotations[m_draggingTextIdx].pos = imgPos - m_textDragOffset;
        markCacheDirty();
        update();
        return;
    }

    if (m_resizingText && m_resizingTextIdx >= 0 && m_resizingTextIdx < m_annotations.size()) {
        Annotation& ann = m_annotations[m_resizingTextIdx];
        qreal curDist = (imgPos - ann.pos).manhattanLength();
        if (m_resizeStartDist > 0) {
            qreal ratio = curDist / m_resizeStartDist;
            qreal newSize = qBound(8.0, m_resizeStartSize * ratio, 200.0);
            ann.font.setPointSizeF(newSize);
            markCacheDirty();
            update();
        }
        return;
    }

    
    if (m_draggingArrow && m_draggingArrowIdx >= 0 && m_draggingArrowIdx < m_annotations.size()) {
        Annotation& ann = m_annotations[m_draggingArrowIdx];
        if (m_draggingArrowEndpoint == "start")
            ann.start = imgPos;
        else
            ann.end = imgPos;
        markCacheDirty();
        update();
        return;
    }

    if (m_drawing) {
        m_drawCurrent = imgPos;
        if (m_currentTool == Tool::Eraser && m_eraserMode == "free") {
            m_freehandPoints.append(imgPos);
            
            {
                QLineF line(m_lastErasePos, imgPos);
                double len = line.length();
                int steps = qMax(1, int(len / qMax(2.0, m_eraserWidth * 0.25)));
                for (int i = 1; i <= steps; ++i) {
                    QPointF pt = m_lastErasePos + (imgPos - m_lastErasePos) * (double(i) / steps);
                    eraseAtPos(pt.toPoint());
                }
            }
            m_lastErasePos = imgPos;
            update();
            return;
        }
        bool hlSketch = (m_currentTool == Tool::Highlighter && m_hlMode == "sketch");
        bool mosaicSketch = (m_currentTool == Tool::Mosaic && !m_mosaicFill);
        if (m_currentTool == Tool::Pen || hlSketch || mosaicSketch)
            m_freehandPoints.append(imgPos);
        update();
    }
}

void StickerWindow::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return;
    if (m_dragging) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
        saveStickerState();return;
    }
    if (m_draggingText) {
        m_draggingText = false;
        m_draggingTextIdx = -1;
        m_textDragOffset = QPoint();
        setCursor(Qt::ArrowCursor);
        return;
    }
    if (m_resizingText) {
        m_resizingText = false;
        m_resizingTextIdx = -1;
        setCursor(Qt::ArrowCursor);
        return;
    }
    if (m_draggingArrow) {
        m_draggingArrow = false;
        m_draggingArrowIdx = -1;
        m_draggingArrowEndpoint.clear();
        setCursor(Qt::ArrowCursor);
        saveStickerState();
        return;
    }
    if (!m_drawing) return;

    if (m_currentTool == Tool::Eraser) {
        m_drawing = false;
        if (m_eraserMode == "fill") {
            QRect r = QRect(m_drawStart, toImagePos(e->pos())).normalized();
            QVector<Annotation> kept;
            for (const auto& ann : m_annotations) {
                if (annIntersectsRect(ann, r)) { m_redoStack.append(ann); continue; }
                kept.append(ann);
            }
            if (kept.size() != m_annotations.size()) {
                m_annotations = kept;
                refreshUndoRedo();
                markCacheDirty();
            }
        }
        m_freehandPoints.clear();
        m_drawStart = QPoint();
        m_drawCurrent = QPoint();
        update();
        saveStickerState();return;
    }

    m_drawing = false;
    finishAnnotation();
    update();
saveStickerState();}

void StickerWindow::finishAnnotation() {
    
    Tool tool = m_currentTool;
    if (tool == Tool::Rect || tool == Tool::Ellipse || tool == Tool::Arrow) {
        if (m_drawStart == m_drawCurrent) return;
        Annotation ann;
        ann.tool = tool;
        ann.start = m_drawStart;
        ann.end = m_drawCurrent;
        ann.color = m_annColor;
        ann.width = (tool == Tool::Arrow) ? m_arrowWidth : m_shapeWidth;
        ann.fill = (tool == Tool::Rect || tool == Tool::Ellipse) ? m_shapeFill : false;
        if (tool == Tool::Arrow) ann.arrowStyle = m_arrowStyle;
        pushAnnotation(ann);
    } else if (tool == Tool::Highlighter && m_hlMode == "box") {
        if (m_drawStart == m_drawCurrent) return;
        Annotation ann;
        ann.tool = Tool::Highlighter;
        ann.hlMode = "box";
        ann.start = m_drawStart;
        ann.end = m_drawCurrent;
        ann.color = m_annColor;
        ann.width = m_highlighterWidth;
        pushAnnotation(ann);
    } else if (tool == Tool::Pen || tool == Tool::Highlighter) {
        if (m_freehandPoints.size() < 2) return;
        Annotation ann;
        ann.tool = tool;
        ann.points = m_freehandPoints;
        ann.color = m_annColor;
        ann.width = (tool == Tool::Pen) ? m_penWidth : m_highlighterWidth;
        if (tool == Tool::Highlighter) ann.hlMode = m_hlMode;
        pushAnnotation(ann);
    } else if (tool == Tool::Mosaic) {
        if (m_mosaicFill) {
            if (m_drawStart == m_drawCurrent) return;
            Annotation ann;
            ann.tool = Tool::Mosaic;
            ann.mosaicFill = true;
            ann.start = m_drawStart;
            ann.end = m_drawCurrent;
            ann.width = m_mosaicBlock;
            ann.blockSize = m_mosaicBlock;
    ann.mosaicOpacity = m_mosaicOpacity;
            ann.mosaicMode = m_mosaicMode;
            pushAnnotation(ann);
        } else {
            if (m_freehandPoints.size() < 1) return;
            Annotation ann;
            ann.tool = Tool::Mosaic;
            ann.points = m_freehandPoints;
            ann.width = m_mosaicBlock;
            ann.blockSize = m_mosaicBlock;
    ann.mosaicOpacity = m_mosaicOpacity;
            ann.mosaicMode = m_mosaicMode;
            pushAnnotation(ann);
        }
    }
    m_freehandPoints.clear();
    m_drawStart = QPoint();
    m_drawCurrent = QPoint();
}

void StickerWindow::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return;
    QPoint imgPos = toImagePos(e->pos());
    for (int i = m_annotations.size() - 1; i >= 0; i--) {
        const Annotation& ann = m_annotations[i];
        if (ann.tool == Tool::Text && hitTest(ann, imgPos)) {
            startInlineEdit(imgPos, i);
            return;
        }
    }
    if (m_currentTool == Tool::None) close();
}

void StickerWindow::adjustToolWidth(int delta) {
    if (m_currentTool == Tool::None) return;
    int step = delta > 0 ? 1 : -1;
    QString toolName;
    switch (m_currentTool) {
        case Tool::Pen:
            m_penWidth = qBound(1, m_penWidth + step, 50);
            toolName = I18n::t("sticker.pen", QStringLiteral("画笔"));
            syncWidthLabel();
            showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_penWidth), 800);
            break;
        case Tool::Highlighter:
            m_highlighterWidth = qBound(1, m_highlighterWidth + step, 50);
            toolName = I18n::t("sticker.highlighter", QStringLiteral("记号笔"));
            syncWidthLabel();
            showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_highlighterWidth), 800);
            break;
        case Tool::Arrow:
            m_arrowWidth = qBound(1, m_arrowWidth + step, 50);
            toolName = I18n::t("sticker.arrow", QStringLiteral("箭头"));
            syncWidthLabel();
            showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_arrowWidth), 800);
            break;
        case Tool::Rect:
        case Tool::Ellipse:
            m_shapeWidth = qBound(1, m_shapeWidth + step, 50);
            toolName = I18n::t("sticker.shape", QStringLiteral("形状"));
            syncWidthLabel();
            showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_shapeWidth), 800);
            break;
        case Tool::Mosaic:
            m_mosaicBlock = qBound(2, m_mosaicBlock + step, 50);
            toolName = I18n::t("sticker.mosaic", QStringLiteral("马赛克"));
            syncWidthLabel();
            showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_mosaicBlock), 800);
            break;
        case Tool::Eraser:
            m_eraserWidth = qBound(2, m_eraserWidth + step, 100);
            toolName = I18n::t("sticker.eraser", QStringLiteral("橡皮擦"));
            syncWidthLabel();
            showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_eraserWidth), 800);
            break;
        case Tool::Text: {
            int ps = m_annFont.pointSize() > 0 ? m_annFont.pointSize() : 16;
            ps = qBound(8, ps + step, 200);
            m_annFont.setPointSize(ps);
            toolName = I18n::t("sticker.text", QStringLiteral("文字"));
            syncWidthLabel();
            if (m_fontPanel) m_fontPanel->setPointSize(ps);
            showToast(QStringLiteral("%1 %2px").arg(toolName).arg(ps), 800);
            break;
        }
        default: break;
    }
}

void StickerWindow::wheelEvent(QWheelEvent* e) {
    int delta = e->angleDelta().y();

    // Ctrl+wheel: adjust opacity
    if (e->modifiers() & Qt::ControlModifier) {
        qreal step = 0.05;
        m_opacity += (delta > 0 ? step : -step);
        m_opacity = qBound(0.05, m_opacity, 1.0);
        setWindowOpacity(m_opacity);
        showToast(QStringLiteral("%1 %2%").arg(I18n::t("sticker.info_opacity", QStringLiteral("透明度"))).arg(qRound(m_opacity * 100)), 800);
        saveStickerState();
        e->accept();
        return;
    }

    // Plain wheel (or Shift+wheel) with active tool: adjust tool thickness
    if (m_currentTool != Tool::None && !(e->modifiers() & Qt::ControlModifier)) {
        int step = delta > 0 ? 1 : -1;
        QString toolName;
        switch (m_currentTool) {
            case Tool::Pen:
                m_penWidth = qBound(1, m_penWidth + step, 50);
                toolName = I18n::t("sticker.pen", QStringLiteral("画笔"));
                syncWidthLabel();
                showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_penWidth), 800);
                break;
            case Tool::Highlighter:
                m_highlighterWidth = qBound(2, m_highlighterWidth + step * 2, 80);
                toolName = I18n::t("sticker.highlighter", QStringLiteral("记号笔"));
                syncWidthLabel();
                showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_highlighterWidth), 800);
                break;
            case Tool::Rect:
            case Tool::Ellipse:
                m_shapeWidth = qBound(1, m_shapeWidth + step, 50);
                toolName = I18n::t("sticker.shape", QStringLiteral("形状"));
                syncWidthLabel();
                showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_shapeWidth), 800);
                break;
            case Tool::Arrow:
                m_arrowWidth = qBound(1, m_arrowWidth + step, 50);
                toolName = I18n::t("sticker.arrow", QStringLiteral("箭头"));
                syncWidthLabel();
                showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_arrowWidth), 800);
                break;
            case Tool::Mosaic:
                m_mosaicBlock = qBound(2, m_mosaicBlock + step, 50);
                toolName = I18n::t("sticker.mosaic", QStringLiteral("马赛克"));
                syncWidthLabel();
                showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_mosaicBlock), 800);
                break;
            case Tool::Eraser:
                m_eraserWidth = qBound(5, m_eraserWidth + step * 2, 100);
                toolName = I18n::t("sticker.eraser", QStringLiteral("橡皮擦"));
                syncWidthLabel();
                showToast(QStringLiteral("%1 %2px").arg(toolName).arg(m_eraserWidth), 800);
                break;
            case Tool::Text: {
                int curSize = m_annFont.pointSize() > 0 ? m_annFont.pointSize() : 16;
                int newSize = qBound(8, curSize + step, 200);
                if (m_fontPanel) m_fontPanel->setPointSize(newSize);
                
                toolName = I18n::t("sticker.text", QStringLiteral("文字"));
                syncWidthLabel();
                showToast(QStringLiteral("%1 %2px").arg(toolName).arg(newSize), 800);
                break;
            }
            default:
                break;
        }
        update();
        e->accept();
        return;
    }

    // Plain wheel: zoom sticker (when no tool is active)
    if (m_currentTool != Tool::None) {
        e->accept();
        return;
    }

    qreal factor = delta > 0 ? 1.1 : (1.0 / 1.1);
    qreal newScale = m_scale * factor;
    if (newScale < 0.2) newScale = 0.2;
    if (newScale > 5.0) newScale = 5.0;
    applyScale(newScale);
    showToast(QString::number(qRound(m_scale * 100)) + QStringLiteral("%"), 800);
    saveStickerState();
    e->accept();
}

void StickerWindow::applyScale(qreal factor) {
    m_scale = factor;
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    int newW = qRound(m_originalPixmap.width() / dpr * m_scale);
    int newH = qRound(m_originalPixmap.height() / dpr * m_scale);
    m_pixmap = m_originalPixmap.scaled(qRound(newW * dpr), qRound(newH * dpr), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_pixmap.setDevicePixelRatio(dpr);
    m_imgW = newW;
    m_imgH = newH;
    setFixedSize(m_imgW + SHADOW_MARGIN * 2, m_imgH + SHADOW_MARGIN * 2);
    repositionToolbar();
    markCacheDirty();
    update();
}

void StickerWindow::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Shift) {
        if (m_colorFmt == "RGB") m_colorFmt = "HEX";
        else if (m_colorFmt == "HEX") m_colorFmt = "RGB+HEX";
        else if (m_colorFmt == "RGB+HEX") m_colorFmt = "HEX+RGB";
        else if (m_colorFmt == "HEX+RGB") m_colorFmt = "RGB";
        update();
        return;
    }
    if (e->key() == Qt::Key_Escape) {
        if (m_currentTool != Tool::None) {
            m_currentTool = Tool::None;
            if (m_toolbar) m_toolbar->setActiveTool(QString());
            setCursor(Qt::ArrowCursor);
            if (m_toolbar) m_toolbar->updateWidthLabel(QString(), 0);
        }
    } else if (e->key() == Qt::Key_Z && (e->modifiers() & Qt::ControlModifier)) {
        if (e->modifiers() & Qt::ShiftModifier) redo();
        else undo();
    } else if (e->key() == Qt::Key_Y && (e->modifiers() & Qt::ControlModifier)) {
        redo();
    } else if (e->key() == Qt::Key_Delete) {
        clearAnnotations();
    }
}


void StickerWindow::onToolChanged(const QString& key) {
    // shape/arrow/highlighter: always activate (preserve existing behavior, no toggle-off)
    if (key == "shape") {
        m_currentTool = m_shapeTool;
        if (m_toolbar) m_toolbar->setActiveTool("shape");
        showUnifiedPanel("shape");
        setCursor(Qt::CrossCursor);
        syncWidthLabel();
        return;
    }
    if (m_textEditor) finishInlineEdit();
    if (key == "arrow") {
        m_currentTool = Tool::Arrow;
        if (m_toolbar) m_toolbar->setActiveTool("arrow");
        showUnifiedPanel("arrow");
        setCursor(Qt::CrossCursor);
        syncWidthLabel();
        return;
    }
    if (key == "highlighter") {
        m_currentTool = Tool::Highlighter;
        if (m_toolbar) {
            m_toolbar->setActiveTool("highlighter");
            m_toolbar->setDotSelected(m_hlMode == "sketch");
        }
        showUnifiedPanel("highlighter");
        setCursor(Qt::CrossCursor);
        syncWidthLabel();
        return;
    }
    
    if (m_textEditor) finishInlineEdit();

    // pen/mosaic/text/eraser: toggle on/off
    Tool target = Tool::None;
    QString toolKey;
    if (key == "pen")          { target = Tool::Pen;     toolKey = "pen"; }
    else if (key == "mosaic")  { target = Tool::Mosaic;  toolKey = "mosaic"; }
    else if (key == "text")    { target = Tool::Text;    toolKey = "text"; }
    else if (key == "eraser")  { target = Tool::Eraser;  toolKey = "eraser"; }
    if (m_currentTool == target) {
        m_currentTool = Tool::None;
        toolKey.clear();
    } else {
        m_currentTool = target;
    }
    if (m_toolbar) m_toolbar->setActiveTool(
        m_currentTool == Tool::Pen ? "pen" :
        m_currentTool == Tool::Mosaic ? "mosaic" :
        m_currentTool == Tool::Text ? "text" :
        m_currentTool == Tool::Eraser ? "eraser" : QString());

    if (m_currentTool == Tool::None) {
        hideUnifiedPanel();
        setCursor(Qt::ArrowCursor);
        if (m_toolbar) m_toolbar->updateWidthLabel(QString(), 0);
        repositionToolbar();
    } else {
        showUnifiedPanel(toolKey);
        setCursor(Qt::CrossCursor);
        syncWidthLabel();
        if (m_currentTool == Tool::Eraser && m_toolbar)
            m_toolbar->setDotSelected(m_eraserMode == "free");
        else if (m_currentTool == Tool::Mosaic && m_toolbar)
            m_toolbar->setDotSelected(!m_mosaicFill);
    }
}

void StickerWindow::onAction(const QString& key) {
    if (key == "undo") undo();
    else if (key == "redo") redo();
    else if (key == "exit") { m_showToolbar = false; updateToolbar(); }
    else if (key == "color") { /* color button now redundant: unified panel always carries color section */ }
    else if (key == "pin") { pinToDesktop(); }
    else if (key == "save") saveToFile();
    else if (key == "copy") copyToClipboard();
    else if (key == "clear") clearAnnotations();
}

void StickerWindow::setActiveTool(Tool tool) {
    m_currentTool = tool;
    if (!m_toolbar) return;
    QString key;
    switch (tool) {
    case Tool::Rect: key = "rect"; break;
    case Tool::Ellipse: key = "ellipse"; break;
    case Tool::Arrow: key = "arrow"; break;
    case Tool::Pen: key = "pen"; break;
    case Tool::Highlighter: key = "highlighter"; break;
    case Tool::Mosaic: key = "mosaic"; break;
    case Tool::Text: key = "text"; break;
    case Tool::Eraser: key = "eraser"; break;
    default: break;
    }
    m_toolbar->setActiveTool(key);
    if (tool == Tool::None) {
        hideUnifiedPanel();
        setCursor(Qt::ArrowCursor);
    } else {
        // Rect/Ellipse share the shape panel
        QString panelKey = (tool == Tool::Rect || tool == Tool::Ellipse) ? QStringLiteral("shape") : key;
        showUnifiedPanel(panelKey);
        setCursor(Qt::CrossCursor);
    }
}

void StickerWindow::syncWidthLabel() {
    if (!m_toolbar) return;
    Tool tool = m_currentTool;
    if (tool == Tool::None) { m_toolbar->updateWidthLabel(QString(), 0); return; }
    QString name; int w = 0;
    if (tool == Tool::Pen) { name = "pen"; w = m_penWidth; }
    else if (tool == Tool::Highlighter) { name = "highlighter"; w = m_highlighterWidth; }
    else if (tool == Tool::Rect) { name = "rect"; w = m_shapeWidth; }
    else if (tool == Tool::Ellipse) { name = "ellipse"; w = m_shapeWidth; }
    else if (tool == Tool::Arrow) { name = "arrow"; w = m_arrowWidth; }
    
    else if (tool == Tool::Mosaic) { name = "mosaic"; w = m_mosaicBlock; }
    else if (tool == Tool::Eraser) { name = "eraser"; w = m_eraserWidth; }
    
    if (!name.isEmpty()) m_toolbar->updateWidthLabel(name, w);
    else m_toolbar->updateWidthLabel(QString(), 0);
    // Width label visibility change alters toolbar height; reposition sub-panels
    repositionSubPanels();
}

double StickerWindow::distToLine(const QPoint& p, const QPoint& a, const QPoint& b) const {
    int dx = b.x() - a.x();
    int dy = b.y() - a.y();
    if (dx == 0 && dy == 0) return std::hypot(double(p.x() - a.x()), double(p.y() - a.y()));
    double t = (std::max)(0.0, (std::min)(1.0, double((p.x() - a.x()) * dx + (p.y() - a.y()) * dy) / (dx * dx + dy * dy)));
    double px = a.x() + t * dx;
    double py = a.y() + t * dy;
    return std::hypot(p.x() - px, p.y() - py);
}

void StickerWindow::eraseAtPos(const QPoint& imgPos) {
    if (m_annotations.isEmpty()) return;
    int r = m_eraserWidth;
    QList<Annotation> newAnns;
    bool changed = false;
    for (const auto& ann : m_annotations) {
        switch (ann.tool) {
        case Tool::Pen:
        case Tool::Highlighter:
            if (ann.hlMode != "box") {
                bool touched = false;
                for (const QPoint& p : ann.points) {
                    if ((p - imgPos).manhattanLength() < r) { touched = true; break; }
                }
                if (!touched) { newAnns.append(ann); }
                else {
                    changed = true;
                    auto segs = splitPathAtCircle(ann.points, imgPos, r);
                    for (const auto& seg : segs) {
                        if (seg.size() >= 2) {
                            Annotation na = ann; na.points = seg; newAnns.append(na);
                        }
                    }
                }
            } else {
                if (!QRect(ann.start, ann.end).normalized().contains(imgPos)) newAnns.append(ann);
                else changed = true;
            }
            break;
        case Tool::Mosaic:
            if (!ann.mosaicFill) {
                bool touched = false;
                QList<QPoint> filtered;
                for (const QPoint& p : ann.points) {
                    if ((p - imgPos).manhattanLength() < r) { touched = true; }
                    else filtered.append(p);
                }
                if (!touched) newAnns.append(ann);
                else {
                    changed = true;
                    if (!filtered.isEmpty()) { Annotation na = ann; na.points = filtered; newAnns.append(na); }
                }
            } else {
                if (!QRect(ann.start, ann.end).normalized().contains(imgPos)) newAnns.append(ann);
                else changed = true;
            }
            break;
        case Tool::Arrow: {
            
            if (distToLine(imgPos, ann.start, ann.end) >= r) { newAnns.append(ann); }
            else { changed = true; }
            break;
        }
        case Tool::Rect:
        case Tool::Ellipse: {
            
            QRect rect = QRect(ann.start, ann.end).normalized();
            auto edges = rectEdges(rect);
            bool touched = false;
            for (const auto& edge : edges) {
                if (distToLine(imgPos, edge.first, edge.second) < r) { touched = true; break; }
            }
            if (!touched) { newAnns.append(ann); }
            else { changed = true; }
            break;
        }
        case Tool::Text:
            if (!hitTest(ann, imgPos)) newAnns.append(ann);
            else changed = true;
            break;
        default:
            newAnns.append(ann);
            break;
        }
    }
    if (changed) {
        m_annotations = newAnns;
        m_redoStack.clear();
        refreshUndoRedo();
        markCacheDirty();
        update();
    }
}

void StickerWindow::eraseBox(const QRect& rect) {
    if (m_annotations.isEmpty()) return;
    QList<Annotation> kept;
    bool changed = false;
    for (const auto& ann : m_annotations) {
        if (annIntersectsRect(ann, rect)) changed = true;
        else kept.append(ann);
    }
    if (changed) {
        m_annotations = kept;
        m_redoStack.clear();
        refreshUndoRedo();
        markCacheDirty();
        update();
    }
}

QList<Annotation> StickerWindow::splitArrow(const Annotation& ann, const QPoint& pos, int r) const {
    QList<Annotation> results;
    QPoint start = ann.start;
    QPoint end = ann.end;
    int dx = end.x() - start.x();
    int dy = end.y() - start.y();
    double length = std::hypot(double(dx), double(dy));
    if (length < 1e-6) return results;
    double ux = dx / length;
    double uy = dy / length;
    double t = ((pos.x() - start.x()) * dx + (pos.y() - start.y()) * dy) / (double(dx) * dx + double(dy) * dy);
    if (t < 0.0) t = 0.0; if (t > 1.0) t = 1.0;
    double eraseT = (length > 0) ? r / length : 0.0;
    double tLo = t - eraseT; if (tLo < 0.0) tLo = 0.0;
    double tHi = t + eraseT; if (tHi > 1.0) tHi = 1.0;
    if (tLo > 0.01) {
        Annotation na = ann;
        na.end = QPoint(int(start.x() + ux * length * tLo), int(start.y() + uy * length * tLo));
        results.append(na);
    }
    if (tHi < 0.99) {
        Annotation na = ann;
        na.start = QPoint(int(start.x() + ux * length * tHi), int(start.y() + uy * length * tHi));
        results.append(na);
    }
    return results;
}

QList<QVector<QPoint>> StickerWindow::splitPathAtCircle(const QVector<QPoint>& points, const QPoint& center, int radius) {
    QList<QVector<QPoint>> segments;
    QVector<QPoint> current;
    for (const auto& p : points) {
        if ((p - center).manhattanLength() < radius) {
            if (current.size() >= 2) segments.append(current);
            current.clear();
        } else {
            current.append(p);
        }
    }
    if (current.size() >= 2) segments.append(current);
    return segments;
}

bool StickerWindow::annIntersectsRect(const Annotation& ann, const QRect& rect) const {
    switch (ann.tool) {
    case Tool::Rect:
    case Tool::Ellipse:
        return QRect(ann.start, ann.end).normalized().intersects(rect);
    case Tool::Arrow:
        return QRect(ann.start, ann.end).normalized().intersects(rect);
    case Tool::Highlighter:
        if (ann.hlMode == "box") return QRect(ann.start, ann.end).normalized().intersects(rect);
        for (const auto& p : ann.points) { if (rect.contains(p)) return true; }
        for (int i = 0; i < ann.points.size() - 1; i++) {
            if (lineRectIntersect(ann.points[i], ann.points[i + 1], rect)) return true;
        }
        return false;
    case Tool::Pen:
        for (const auto& p : ann.points) { if (rect.contains(p)) return true; }
        for (int i = 0; i < ann.points.size() - 1; i++) {
            if (lineRectIntersect(ann.points[i], ann.points[i + 1], rect)) return true;
        }
        return false;
    case Tool::Mosaic:
        if (ann.mosaicFill) return QRect(ann.start, ann.end).normalized().intersects(rect);
        for (const auto& p : ann.points) { if (rect.contains(p)) return true; }
        return false;
    case Tool::Text: {
        QRect tr = textBbox(ann);
        return tr.intersects(rect);
    }
    default: return false;
    }
}

bool StickerWindow::lineRectIntersect(const QPoint& p1, const QPoint& p2, const QRect& rect) {
    if (rect.contains(p1) || rect.contains(p2)) return true;
    QList<QPair<QPoint, QPoint>> edges;
    edges << qMakePair(rect.topLeft(), rect.topRight())
          << qMakePair(rect.topRight(), rect.bottomRight())
          << qMakePair(rect.bottomRight(), rect.bottomLeft())
          << qMakePair(rect.bottomLeft(), rect.topLeft());
    for (const auto& e : edges) {
        if (segmentsIntersect(p1, p2, e.first, e.second)) return true;
    }
    return false;
}

bool StickerWindow::segmentsIntersect(const QPoint& p1, const QPoint& p2, const QPoint& p3, const QPoint& p4) {
    auto cross = [](const QPoint& o, const QPoint& a, const QPoint& b) -> double {
        return double(a.x() - o.x()) * (b.y() - o.y()) - double(a.y() - o.y()) * (b.x() - o.x());
    };
    double d1 = cross(p3, p4, p1);
    double d2 = cross(p3, p4, p2);
    double d3 = cross(p1, p2, p3);
    double d4 = cross(p1, p2, p4);
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) return true;
    return false;
}

QList<QPair<QPoint, QPoint>> StickerWindow::rectEdges(const QRect& rect) {
    QList<QPair<QPoint, QPoint>> edges;
    edges << qMakePair(rect.topLeft(), rect.topRight())
          << qMakePair(rect.topRight(), rect.bottomRight())
          << qMakePair(rect.bottomRight(), rect.bottomLeft())
          << qMakePair(rect.bottomLeft(), rect.topLeft());
    return edges;
}

QString StickerWindow::hitArrowEndpoint(const Annotation& ann, const QPoint& pos, int threshold) const {
    if ((pos - ann.start).manhattanLength() < threshold) return "start";
    if ((pos - ann.end).manhattanLength() < threshold) return "end";
    return QString();
}

bool StickerWindow::hitTest(const Annotation& ann, const QPoint& pos) const {
    int threshold = 10;
    switch (ann.tool) {
    case Tool::Rect:
    case Tool::Ellipse: {
        QRect r = QRect(ann.start, ann.end).normalized();
        QRect outer = r.adjusted(-threshold, -threshold, threshold, threshold);
        QRect inner = r.adjusted(threshold, threshold, -threshold, -threshold);
        return outer.contains(pos) && !inner.contains(pos);
    }
    case Tool::Arrow:
        return distToLine(pos, ann.start, ann.end) < threshold;
    case Tool::Highlighter:
        if (ann.hlMode == "box") return QRect(ann.start, ann.end).normalized().contains(pos);
        for (int i = 0; i < ann.points.size() - 1; i++) {
            if (distToLine(pos, ann.points[i], ann.points[i + 1]) < threshold) return true;
        }
        return false;
    case Tool::Pen:
        for (int i = 0; i < ann.points.size() - 1; i++) {
            if (distToLine(pos, ann.points[i], ann.points[i + 1]) < threshold) return true;
        }
        return false;
    case Tool::Mosaic:
        if (ann.mosaicFill) return QRect(ann.start, ann.end).normalized().contains(pos);
        for (const auto& pt : ann.points) {
            if ((pos - pt).manhattanLength() < ann.width) return true;
        }
        return false;
    case Tool::Text: {
        QRect tr = textBbox(ann);
        return tr.contains(pos);
    }
    default: return false;
    }
}

QRect StickerWindow::textBbox(const Annotation& ann) const {
    QFontMetrics fm(ann.font);
    int tw = fm.horizontalAdvance(ann.text);
    int th = fm.ascent();
    return QRect(ann.pos.x() - 2, ann.pos.y() - th - 2, tw + 4, th + fm.descent() + 4);
}

QRect StickerWindow::textHandleRect(const Annotation& ann) const {
    QRect box = textBbox(ann);
    int hs = 8;
    return QRect(box.right() - hs / 2, box.bottom() - hs / 2, hs, hs);
}

bool StickerWindow::hitTextHandle(const Annotation& ann, const QPoint& pos) const {
    return textHandleRect(ann).contains(pos);
}

void StickerWindow::startInlineEdit(const QPoint& imgPos, int existingIdx) {
    finishInlineEdit();
    QFont font = m_annFont;
    QString oldText;
    QPoint editorBase;
    int editorW = 120;
    int editorH;
    if (existingIdx >= 0 && existingIdx < m_annotations.size()) {
        const Annotation& ann = m_annotations[existingIdx];
        oldText = ann.text;
        font = ann.font;
        QPoint winPos = toWindowPos(ann.pos);
        QFontMetrics fm(font);
        editorBase = QPoint(winPos.x(), winPos.y() - fm.ascent());
        editorW = std::max(100, fm.horizontalAdvance(oldText) + 20);
        editorH = fm.height() + 4;
    } else {
        oldText.clear();
        QPoint winPos = toWindowPos(imgPos);
        QFontMetrics fm(font);
        editorBase = winPos;
        editorH = fm.height() + 4;
    }
    QLineEdit* editor = new QLineEdit(this);
    editor->setText(oldText);
    editor->setFont(font);
    editor->setGeometry(editorBase.x(), editorBase.y(), editorW, editorH);
    editor->setStyleSheet("QLineEdit { background: white; border: 2px solid #1a7eff; color: #333; padding: 0px 2px; }");
    editor->selectAll();
    editor->setFocus();
    editor->show();
    editor->raise();
    connect(editor, &QLineEdit::returnPressed, this, &StickerWindow::finishInlineEdit);
    editor->installEventFilter(this);
    m_textEditor = editor;
    m_textEditorIdx = existingIdx;
    m_textEditorPos = imgPos;
}

void StickerWindow::finishInlineEdit() {
    if (!m_textEditor) return;
    QString text = m_textEditor->text();
    int idx = m_textEditorIdx;
    m_textEditor->close();
    m_textEditor->deleteLater();
    m_textEditor = nullptr;
    m_textEditorIdx = -1;
    if (idx >= 0 && idx < m_annotations.size()) {
        if (!text.isEmpty()) m_annotations[idx].text = text;
        else m_annotations.removeAt(idx);
    } else if (!text.isEmpty()) {
        Annotation ann;
        ann.tool = Tool::Text;
        ann.pos = m_textEditorPos;
        ann.text = text;
        ann.color = m_annColor;
        ann.font = m_annFont;
        m_annotations.append(ann);
        m_redoStack.clear();
    }
    refreshUndoRedo();
    markCacheDirty();
    update();
}

void StickerWindow::cancelInlineEdit() {
    if (!m_textEditor) return;
    m_textEditor->close();
    m_textEditor->deleteLater();
    m_textEditor = nullptr;
    m_textEditorIdx = -1;
    update();
}

bool StickerWindow::eventFilter(QObject* obj, QEvent* e) {
    if (obj == m_textEditor && e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Escape) { cancelInlineEdit(); return true; }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) { finishInlineEdit(); return true; }
    }
    return QWidget::eventFilter(obj, e);
}


void StickerWindow::pushAnnotation(const Annotation& ann) {
    m_annotations.append(ann);
    m_redoStack.clear();
    refreshUndoRedo();
    markCacheDirty();
    saveStickerState();
}

void StickerWindow::undo() {
    if (m_annotations.isEmpty()) return;
    Annotation ann = m_annotations.takeLast();
    m_redoStack.append(ann);
    refreshUndoRedo();
    markCacheDirty();
    update();
    saveStickerState();
}

void StickerWindow::redo() {
    if (m_redoStack.isEmpty()) return;
    Annotation ann = m_redoStack.takeLast();
    m_annotations.append(ann);
    refreshUndoRedo();
    markCacheDirty();
    update();
    saveStickerState();
}

void StickerWindow::clearAnnotations() {
    if (m_annotations.isEmpty()) return;
    for (const auto& a : m_annotations) m_redoStack.append(a);
    m_annotations.clear();
    refreshUndoRedo();
    markCacheDirty();
    update();
    saveStickerState();
}


void StickerWindow::showUnifiedPanel(const QString& toolKey) {
    if (!m_toolbar) return;
    QWidget* prop = nullptr;
    bool showColor = false;
    if      (toolKey == "text")        { prop = m_fontPanel;    showColor = true; }
    else if (toolKey == "pen")         { prop = nullptr;       showColor = true; }
    else if (toolKey == "shape")       { prop = m_shapePanel;   showColor = true; }
    else if (toolKey == "arrow")       { prop = m_arrowPanel;   showColor = true; }
    else if (toolKey == "highlighter") { prop = m_hlPanel;       showColor = true; }
    else if (toolKey == "mosaic")      { prop = m_mosaicPanel;  showColor = false; }
    else if (toolKey == "eraser")      { prop = m_eraserPanel;  showColor = false; }
    m_toolbar->setPropPanel(prop);
    m_toolbar->setColorPanelVisible(showColor);
    m_toolbar->updateDotColor(m_annColor);
    repositionToolbar();
    m_toolbar->showUnifiedPanel();
    m_toolbar->raise();
}

void StickerWindow::hideUnifiedPanel() {
    if (m_toolbar) m_toolbar->hideUnifiedPanel();
}

void StickerWindow::onColorPicked(const QColor& c) {
    m_annColor = c;
    if (m_toolbar) m_toolbar->updateDotColor(c);
}

void StickerWindow::onShapeSelected(Tool tool, bool fill) {
    m_shapeTool = tool;
    m_shapeFill = fill;
    m_currentTool = tool;
    if (m_shapePanel) m_shapePanel->setCurrent(tool, fill);
    if (m_toolbar) m_toolbar->setActiveTool("shape");
    setCursor(Qt::CrossCursor);
    syncWidthLabel();
}

void StickerWindow::onArrowSelected(ArrowStyle style) {
    m_arrowStyle = style;
    m_currentTool = Tool::Arrow;
    if (m_arrowPanel) m_arrowPanel->setCurrent(style);
    if (m_toolbar) m_toolbar->setActiveTool("arrow");
    setCursor(Qt::CrossCursor);
    syncWidthLabel();
}

void StickerWindow::onHlSelected(const QString& mode) {
    m_hlMode = mode;
    m_currentTool = Tool::Highlighter;
    if (m_hlPanel) m_hlPanel->setCurrent(mode);
    if (m_toolbar) {
        m_toolbar->setActiveTool("highlighter");
        m_toolbar->setDotSelected(mode == "sketch");
    }
    setCursor(Qt::CrossCursor);
    syncWidthLabel();
}

void StickerWindow::onMosaicFill(bool fill) {
    m_mosaicFill = fill;
    if (m_mosaicPanel) m_mosaicPanel->setFill(fill);
    if (m_toolbar) m_toolbar->setDotSelected(!fill);
}

void StickerWindow::onEraserSelected(const QString& mode) {
    m_eraserMode = mode;
    m_currentTool = Tool::Eraser;
    if (m_eraserPanel) m_eraserPanel->setCurrent(mode);
    if (m_toolbar) {
        m_toolbar->setActiveTool("eraser");
        
        m_toolbar->setDotSelected(mode == "free");
    }
    setCursor(Qt::CrossCursor);
    syncWidthLabel();
}


void StickerWindow::bakeAnnotations() {
    // Bake annotations into m_originalPixmap at its FULL PHYSICAL resolution.
    // This preserves original image quality regardless of current zoom level.
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    int origPhysW = m_originalPixmap.width();   // original physical pixels
    int origPhysH = m_originalPixmap.height();

    QImage full(origPhysW, origPhysH, QImage::Format_ARGB32);
    full.fill(Qt::transparent);
    QPainter p(&full);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Draw original image at full resolution
    p.drawPixmap(0, 0, origPhysW, origPhysH, m_originalPixmap);

    // Draw annotations: scale from display logical coords to original physical coords
    // Annotations are in display logical coordinates (relative to m_imgW x m_imgH)
    qreal scaleX = (m_imgW > 0) ? (qreal)origPhysW / m_imgW : 1.0;
    qreal scaleY = (m_imgH > 0) ? (qreal)origPhysH / m_imgH : 1.0;
    p.save();
    p.scale(scaleX, scaleY);
    p.translate(-SHADOW_MARGIN, -SHADOW_MARGIN);  // offset toWindowPos which adds SHADOW_MARGIN
    for (const auto& ann : m_annotations) drawAnnotation(p, ann);
    p.restore();
    p.end();

    QPixmap baked = QPixmap::fromImage(full);
    m_originalPixmap = baked;
    m_originalPixmap.setDevicePixelRatio(dpr);
    // Ensure m_origImgW/H match original physical dimensions
    m_origImgW = qMax(1, qRound(origPhysW / dpr));
    m_origImgH = qMax(1, qRound(origPhysH / dpr));

    // Regenerate display pixmap at current scale from updated original
    int dispPhysW = qMax(1, qRound(origPhysW * m_scale));
    int dispPhysH = qMax(1, qRound(origPhysH * m_scale));
    m_pixmap = m_originalPixmap.scaled(dispPhysW, dispPhysH,
                                       Qt::IgnoreAspectRatio,
                                       m_smoothScaling ? Qt::SmoothTransformation : Qt::FastTransformation);
    m_pixmap.setDevicePixelRatio(dpr);
    // Update display dimensions and window size to match current scale
    m_imgW = qMax(1, qRound(dispPhysW / dpr));
    m_imgH = qMax(1, qRound(dispPhysH / dpr));
    setFixedSize(m_imgW + SHADOW_MARGIN * 2, m_imgH + SHADOW_MARGIN * 2);
    update();
    m_annotations.clear();
    m_redoStack.clear();
    refreshUndoRedo();
    markCacheDirty();
    saveStickerState();
}

void StickerWindow::saveToFile() {
    // Render at ORIGINAL physical resolution to avoid blur
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    int origPhysW = m_originalPixmap.width();
    int origPhysH = m_originalPixmap.height();
    QImage full(origPhysW, origPhysH, QImage::Format_ARGB32);
    full.fill(Qt::transparent);
    QPainter p(&full);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPixmap(0, 0, origPhysW, origPhysH, m_originalPixmap);
    // Scale annotations from display coords to original coords
    qreal scaleX = (m_imgW > 0) ? (qreal)origPhysW / m_imgW : 1.0;
    qreal scaleY = (m_imgH > 0) ? (qreal)origPhysH / m_imgH : 1.0;
    p.save();
    p.scale(scaleX, scaleY);
    p.translate(-SHADOW_MARGIN, -SHADOW_MARGIN);  // offset toWindowPos which adds SHADOW_MARGIN
    for (const auto& ann : m_annotations) drawAnnotation(p, ann);
    p.restore();
    p.end();
    QImage result = full;

    QString path = QFileDialog::getSaveFileName(this, I18n::t("sticker.save", QStringLiteral("\u4fdd\u5b58")),
        QString(), QStringLiteral("PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)"));
    if (!path.isEmpty()) {
        result.save(path);
        m_toastText = I18n::t("sticker.saved", QStringLiteral("\u5df2\u4fdd\u5b58"));
        m_toastTimer->start(1500);
        update();
    }
}

void StickerWindow::copyToClipboard() {
    // Render at ORIGINAL physical resolution to avoid blur
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    int origPhysW = m_originalPixmap.width();
    int origPhysH = m_originalPixmap.height();
    QImage full(origPhysW, origPhysH, QImage::Format_ARGB32);
    full.fill(Qt::transparent);
    QPainter p(&full);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPixmap(0, 0, origPhysW, origPhysH, m_originalPixmap);
    qreal scaleX = (m_imgW > 0) ? (qreal)origPhysW / m_imgW : 1.0;
    qreal scaleY = (m_imgH > 0) ? (qreal)origPhysH / m_imgH : 1.0;
    p.save();
    p.scale(scaleX, scaleY);
    p.translate(-SHADOW_MARGIN, -SHADOW_MARGIN);  // offset toWindowPos which adds SHADOW_MARGIN
    for (const auto& ann : m_annotations) drawAnnotation(p, ann);
    p.restore();
    p.end();
    QApplication::clipboard()->setImage(full);
    m_toastText = I18n::t("sticker.copied", QStringLiteral("\u5df2\u590d\u5236"));
    m_toastTimer->start(1500);
    update();
    saveStickerState();
}

void StickerWindow::pinToDesktop() {
    closeAllPanels();
    // bakeAnnotations preserves current m_scale and updates m_origImgW,
    // m_pixmap, m_imgW, setFixedSize at current zoom level.
    // Do NOT reset m_scale to 1.0 - that would enlarge the image.
    bakeAnnotations();
    m_currentTool = Tool::None;
    if (m_toolbar) m_toolbar->setActiveTool(QString());
    m_selected = false;
    updateToolbar();
    repositionToolbar();
    markCacheDirty();
    update();
    setTopmost(true);
    saveStickerState();
}

void StickerWindow::contextMenuEvent(QContextMenuEvent* e) {
    QMenu menu(this);
    menu.setToolTipsVisible(true);
    bool dark = UiTheme::is_dark_theme();
    
    QString menuQss = dark ?
        "QMenu { background: #2d2d2d; color: #ffffff; border: 1px solid #3d3d3d; padding: 2px 0; }"
        "QMenu::item { padding: 3px 18px 3px 6px; margin: 0 2px; border-radius: 3px; }"
        "QMenu::item:selected { background: #404040; }"
        "QMenu::icon { margin-left: 2px; margin-right: 3px; }"
        "QMenu::item:disabled { color: #888888; }"
        "QMenu::separator { height: 1px; background: #555555; margin: 2px 6px; }"
        :
        "QMenu { background: #ffffff; color: #333333; border: 1px solid #cccccc; padding: 2px 0; }"
        "QMenu::item { padding: 3px 18px 3px 6px; margin: 0 2px; border-radius: 3px; }"
        "QMenu::item:selected { background: #e0e0e0; }"
        "QMenu::icon { margin-left: 2px; margin-right: 3px; }"
        "QMenu::item:disabled { color: #999999; }"
        "QMenu::separator { height: 1px; background: #cccccc; margin: 2px 6px; }";
    menu.setStyleSheet(menuQss);

    bool isOriginal = qAbs(m_scale - 1.0) < 0.01;
    QIcon checkIcon = getCheckIcon();

    
    menu.addAction(I18n::t("sticker.copy_image", QStringLiteral("复制图像")), this, &StickerWindow::copyToClipboard);
    if (!isOriginal) {
        menu.addAction(I18n::t("sticker.copy_original_image", QStringLiteral("复制原始大小图像")), this, &StickerWindow::copyOriginalToClipboard);
    }
    
    menu.addAction(I18n::t("sticker.save_image_as", QStringLiteral("图像另存为...")), this, &StickerWindow::saveToFile);
    if (!isOriginal) {
        menu.addAction(I18n::t("sticker.save_original_image_as", QStringLiteral("原始大小图像另存为...")), this, &StickerWindow::saveOriginalToFile);
    }

    menu.addSeparator();

    // OCR / QR
    QString modelPath = QDir(QCoreApplication::applicationDirPath()).filePath("model/MediaOCR-Base-v1.mtk-model");
    bool hasModel = QFileInfo::exists(modelPath);
    auto* ocrAct = menu.addAction(I18n::t("sticker.ocr", QStringLiteral("OCR文字识别")), this, [this](){
        QImage img = getRenderedPixmap().toImage();
        for (QWidget* w : QApplication::topLevelWidgets()) {
            SnipOCRApp* app = qobject_cast<SnipOCRApp*>(w);
            if (app) {
                showToast(I18n::t("sticker.ocr_in_progress", QStringLiteral("正在OCR...")), 0);
                app->ocrImage(img);
                return;
            }
        }
        showToast(I18n::t("sticker.ocr_not_available", QStringLiteral("OCR不可用")), 1500);
    });
    if (!hasModel) {
        ocrAct->setEnabled(false);
        ocrAct->setToolTip(I18n::t("sticker.ocr_no_model", QStringLiteral("当前没有模型，暂不支持使用")));
    }
    menu.addAction(I18n::t("sticker.qr", QStringLiteral("二维码识别")), this, [this](){
        QImage img = getRenderedPixmap().toImage();
        for (QWidget* w : QApplication::topLevelWidgets()) {
            SnipOCRApp* app = qobject_cast<SnipOCRApp*>(w);
            if (app) {
                app->qrImage(img);
                return;
            }
        }
        showToast(I18n::t("sticker.qr_not_available", QStringLiteral("QR不可用")), 1500);
    });

    
    auto* actToolbar = new QAction(I18n::t("sticker.show_toolbar", QStringLiteral("显示工具条")), &menu);
    actToolbar->setCheckable(true);
    actToolbar->setChecked(m_showToolbar);
    if (m_showToolbar) actToolbar->setIcon(checkIcon);
    connect(actToolbar, &QAction::triggered, this, [this, actToolbar](bool) {
        m_showToolbar = actToolbar->isChecked();
        if (m_showToolbar) {
            setSelected(true);
        } else {
            updateToolbar();
            update();
        }
    });
    menu.addAction(actToolbar);

    
    auto* actShadow = new QAction(I18n::t("sticker.window_shadow", QStringLiteral("窗口阴影")), &menu);
    actShadow->setCheckable(true);
    actShadow->setChecked(m_shadowEnabled);
    if (m_shadowEnabled) actShadow->setIcon(checkIcon);
    connect(actShadow, &QAction::toggled, this, [this](bool checked) { m_shadowEnabled = checked; update(); });
    menu.addAction(actShadow);

    
    auto* zoomMenu = new QMenu(I18n::t("sticker.zoom", QStringLiteral("缩放")), &menu);
    zoomMenu->setStyleSheet(menuQss);
    struct ZoomItem { QString label; qreal val; };
    ZoomItem zoomItems[] = {
        {QStringLiteral("33.3%"), 0.333}, {QStringLiteral("50%"), 0.5},
        {QStringLiteral("100%"), 1.0}, {QStringLiteral("200%"), 2.0}
    };
    for (const auto& zi : zoomItems) {
        auto* actZ = new QAction(zi.label, zoomMenu);
        if (qAbs(m_scale - zi.val) < 0.01) {
            actZ->setIcon(checkIcon);
            actZ->setEnabled(false); 
        }
        connect(actZ, &QAction::triggered, this, [this, zi](){ zoomTo(zi.val); });
        zoomMenu->addAction(actZ);
    }
    zoomMenu->addSeparator();
    auto* actCur = new QAction(QString("%1%  (%2)").arg(int(m_scale * 100)).arg(I18n::t("sticker.current", QStringLiteral("当前"))), zoomMenu);
    actCur->setEnabled(false);
    zoomMenu->addAction(actCur);
    zoomMenu->addSeparator();
    auto* actSmooth = new QAction(I18n::t("sticker.smooth", QStringLiteral("平滑处理")), zoomMenu);
    actSmooth->setCheckable(true);
    actSmooth->setChecked(m_smoothScaling);
    connect(actSmooth, &QAction::triggered, this, [this](){ toggleSmooth(); });
    zoomMenu->addAction(actSmooth);
    menu.addMenu(zoomMenu);

    
    auto* procMenu = new QMenu(I18n::t("sticker.image_process", QStringLiteral("图像处理")), &menu);
    procMenu->setStyleSheet(menuQss);
    auto* actRotL = new QAction(I18n::t("sticker.rotate_left", QStringLiteral("向左旋转")), procMenu);
    connect(actRotL, &QAction::triggered, this, [this](){ rotateImage(false); });
    auto* actRotR = new QAction(I18n::t("sticker.rotate_right", QStringLiteral("向右旋转")), procMenu);
    connect(actRotR, &QAction::triggered, this, [this](){ rotateImage(true); });
    auto* actFlipH = new QAction(I18n::t("sticker.flip_horizontal", QStringLiteral("水平翻转")), procMenu);
    connect(actFlipH, &QAction::triggered, this, [this](){ flipImage(true); });
    auto* actFlipV = new QAction(I18n::t("sticker.flip_vertical", QStringLiteral("垂直翻转")), procMenu);
    connect(actFlipV, &QAction::triggered, this, [this](){ flipImage(false); });
    procMenu->addAction(actRotL);
    procMenu->addAction(actRotR);
    procMenu->addSeparator();
    procMenu->addAction(actFlipH);
    procMenu->addAction(actFlipV);
    menu.addMenu(procMenu);

    menu.addSeparator();

    
    menu.addAction(I18n::t("sticker.paste", QStringLiteral("粘贴")), this, &StickerWindow::pasteFromClipboard);
    menu.addAction(I18n::t("sticker.replace_with_file", QStringLiteral("替换为文件...")), this, &StickerWindow::replaceWithFile);

    menu.addSeparator();

    
    bool hasFile = !m_originalFilePath.isEmpty() ||
                   (!m_historyFilePath.isEmpty() && QFile::exists(m_historyFilePath));
    auto* actView = menu.addAction(I18n::t("sticker.view_in_folder", QStringLiteral("在文件夹中查看")), this, &StickerWindow::viewInFolder);
    actView->setEnabled(hasFile);


    auto* moveMenu = new QMenu(I18n::t("sticker.move_to_group", QStringLiteral("移动到分组")), &menu);
    moveMenu->setStyleSheet(menuQss);
    bool hasHistory = !m_historyFilePath.isEmpty() && QFile::exists(m_historyFilePath);
    if (hasHistory) {
        QJsonArray groups = qvariant_cast<QJsonArray>(ConfigManager::instance().get("groups"));
        if (groups.isEmpty()) {
            QJsonObject def; def["id"] = "default"; def["name"] = QStringLiteral("默认");
            groups.append(def);
        }
        QString curGroup;
        if (!m_originalFilePath.isEmpty()) {
            QFileInfo fi(m_originalFilePath);
            QString dirName = fi.dir().dirName();
            if (dirName == "snip") curGroup = "default";
            else curGroup = dirName;
        }
        for (const QJsonValue& gv : groups) {
            QJsonObject g = gv.toObject();
            QString gid = g.value("id").toString();
            QString gname = g.value("name").toString();
            if (gid == "default") gname = I18n::t("tray.group_default_name", QStringLiteral("默认"));
            auto* actG = moveMenu->addAction(gname);
            actG->setCheckable(true);
            actG->setChecked(gid == curGroup);
            if (gid == curGroup) actG->setEnabled(false);
            QString gidCap = gid;
            connect(actG, &QAction::triggered, this, [this, gidCap]() { moveToGroup(gidCap); });
        }
        moveMenu->addSeparator();
        auto* actNew = moveMenu->addAction(I18n::t("sticker.new_group", QStringLiteral("新分组...")), this, &StickerWindow::createGroupAndMove);
        (void)actNew;
    }
    auto* actMove = menu.addMenu(moveMenu);
    actMove->setEnabled(hasHistory);

    menu.addSeparator();

    menu.addSeparator();

    
    auto* actDestroy = menu.addAction(I18n::t("sticker.destroy", QStringLiteral("销毁")), this, &StickerWindow::destroySticker);
    actDestroy->setEnabled(!m_historyFilePath.isEmpty());
    menu.addAction(I18n::t("sticker.close", QStringLiteral("关闭")), this, &QWidget::close);

    menu.addSeparator();

    
    auto* infoMenu = new QMenu(QString("%1 x %2").arg(m_imgW).arg(m_imgH), &menu);
    infoMenu->setStyleSheet(menuQss);
    
    QString groupName = I18n::t("tray.group_default_name", QStringLiteral("默认"));
    if (!m_originalFilePath.isEmpty()) {
        QFileInfo fi(m_originalFilePath);
        QString dirName = fi.dir().dirName();
        QString gid = (dirName == "snip") ? QString("default") : dirName;
        QJsonArray groups = qvariant_cast<QJsonArray>(ConfigManager::instance().get("groups"));
        for (const QJsonValue& gv : groups) {
            QJsonObject g = gv.toObject();
            if (g.value("id").toString() == gid) {
                groupName = g.value("name").toString();
                if (gid == "default") groupName = I18n::t("tray.group_default_name", QStringLiteral("默认"));
                break;
            }
        }
    }
    auto* actInfoGroup = new QAction(QString("%1: %2").arg(I18n::t("sticker.info_group", QStringLiteral("所在组"))).arg(groupName), infoMenu);
    actInfoGroup->setEnabled(false);
    infoMenu->addAction(actInfoGroup);
    auto* actInfoScale = new QAction(QString("%1: %2%").arg(I18n::t("sticker.info_scale", QStringLiteral("缩放"))).arg(int(m_scale * 100)), infoMenu);
    actInfoScale->setEnabled(false);
    infoMenu->addAction(actInfoScale);
    auto* actInfoSize = new QAction(QString("%1: %2 x %3").arg(I18n::t("sticker.info_size", QStringLiteral("尺寸"))).arg(m_imgW).arg(m_imgH), infoMenu);
    actInfoSize->setEnabled(false);
    infoMenu->addAction(actInfoSize);
    infoMenu->addSeparator();
    auto* actInfoOpacity = new QAction(QString("%1: %2%").arg(I18n::t("sticker.info_opacity", QStringLiteral("不透明度"))).arg(int(m_opacity * 100)), infoMenu);
    actInfoOpacity->setEnabled(false);
    infoMenu->addAction(actInfoOpacity);
    QString hMirror = m_hMirrored ? I18n::t("msg.yes", QStringLiteral("是")) : I18n::t("msg.no", QStringLiteral("否"));
    auto* actInfoHmirror = new QAction(QString("%1: %2").arg(I18n::t("sticker.info_hmirror", QStringLiteral("水平镜像"))).arg(hMirror), infoMenu);
    actInfoHmirror->setEnabled(false);
    infoMenu->addAction(actInfoHmirror);
    QString vMirror = m_vMirrored ? I18n::t("msg.yes", QStringLiteral("是")) : I18n::t("msg.no", QStringLiteral("否"));
    auto* actInfoVmirror = new QAction(QString("%1: %2").arg(I18n::t("sticker.info_vmirror", QStringLiteral("垂直镜像"))).arg(vMirror), infoMenu);
    actInfoVmirror->setEnabled(false);
    infoMenu->addAction(actInfoVmirror);
    menu.addMenu(infoMenu);

    menu.exec(e->globalPos());

    // After menu closes, re-raise toolbar if it should be visible.
    // The menu's destruction reshuffles z-order and can bury the toolbar.
    if (m_showToolbar && m_selected && m_toolbar) {
        QTimer::singleShot(0, this, [this]() {
            if (m_selected && m_showToolbar && m_toolbar) {
                repositionToolbar();
                m_toolbar->show();
                m_toolbar->raise();
                m_toolbar->activateWindow();
            }
        });
    }
}

void StickerWindow::closeEvent(QCloseEvent* e) {
    // Python sticker_window.py: only remove desktop state when user-initiated close;
    // programmatic close (keep_state_on_close=true) preserves state for next-launch restore.
    if (!m_keepStateOnClose) {
        removeStickerState();
    } else if (!m_historyFilePath.isEmpty()) {
        saveStickerState();
    }
    emit closed();
    QWidget::closeEvent(e);
}

void StickerWindow::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    repositionToolbar();
}

void StickerWindow::moveEvent(QMoveEvent* e) {
    QWidget::moveEvent(e);
    if (m_toolbar && m_toolbar->isVisible()) repositionToolbar();
}


void StickerWindow::drawMagnifier(QPainter& p) {
    if (!m_magVisible || !m_drawing) return;
    if (m_currentTool == Tool::None) return;

    int ms = m_magSize;
    int zoom = m_magZoom;

    
    QPoint cursor = m_mousePos;
    int mx = cursor.x() + 16;
    int my = cursor.y() + 16;
    if (mx + ms > width()) mx = cursor.x() - ms - 16;
    if (my + ms > height()) my = cursor.y() - ms - 16;
    QRect magRect(mx, my, ms, ms);

    
    p.setPen(QPen(QColor(0, 0, 0, 200), 1));
    p.setBrush(QColor(30, 30, 30, 220));
    p.drawRoundedRect(magRect, 4, 4);

    
    if (!m_staticCache.isNull()) {
        qreal dpr = m_staticCache.devicePixelRatio() > 0 ? m_staticCache.devicePixelRatio() : 1.0;
        int srcSize = ms / zoom;  
        int physSrc = int(srcSize * dpr);
        int cx = int(cursor.x() * dpr);
        int cy = int(cursor.y() * dpr);
        QRect srcPhys(cx - physSrc / 2, cy - physSrc / 2, physSrc, physSrc);
        QRect valid(0, 0, int(m_staticCache.width() * dpr), int(m_staticCache.height() * dpr));
        srcPhys = srcPhys.intersected(valid);
        if (!srcPhys.isEmpty()) {
            QPixmap zoomed = m_staticCache.copy(srcPhys);
            zoomed.setDevicePixelRatio(1.0);  
            zoomed = zoomed.scaled(ms, ms, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            p.drawPixmap(magRect.adjusted(1, 1, -1, -1), zoomed);
        }
    }

    
    p.setPen(QPen(QColor(255, 255, 255, 180), 1, Qt::DashLine));
    int ccx = magRect.center().x();
    int ccy = magRect.center().y();
    p.drawLine(ccx, magRect.top() + 2, ccx, magRect.bottom() - 2);
    p.drawLine(magRect.left() + 2, ccy, magRect.right() - 2, ccy);

    
    // Tick marks on edges (markers every 10px)
    p.setPen(QPen(QColor(255, 255, 255, 100), 1));
    for (int t = 10; t < ms; t += 10) {
        p.drawLine(magRect.left() + t, magRect.top(), magRect.left() + t, magRect.top() + 3);
        p.drawLine(magRect.left() + t, magRect.bottom(), magRect.left() + t, magRect.bottom() - 3);
        p.drawLine(magRect.left(), magRect.top() + t, magRect.left() + 3, magRect.top() + t);
        p.drawLine(magRect.right(), magRect.top() + t, magRect.right() - 3, magRect.top() + t);
    }

    // Corner anchors (L-shaped markers)
    p.setPen(QPen(QColor(26, 174, 255, 220), 2));
    int al = 6;
    p.drawLine(magRect.left(), magRect.top(), magRect.left() + al, magRect.top());
    p.drawLine(magRect.left(), magRect.top(), magRect.left(), magRect.top() + al);
    p.drawLine(magRect.right(), magRect.top(), magRect.right() - al, magRect.top());
    p.drawLine(magRect.right(), magRect.top(), magRect.right(), magRect.top() + al);
    p.drawLine(magRect.left(), magRect.bottom(), magRect.left() + al, magRect.bottom());
    p.drawLine(magRect.left(), magRect.bottom(), magRect.left(), magRect.bottom() - al);
    p.drawLine(magRect.right(), magRect.bottom(), magRect.right() - al, magRect.bottom());
    p.drawLine(magRect.right(), magRect.bottom(), magRect.right(), magRect.bottom() - al);

    // Center point color pickup with configurable format
    QPoint imgPos = toImagePos(cursor);
    if (imgPos.x() >= 0 && imgPos.x() < m_imgW && imgPos.y() >= 0 && imgPos.y() < m_imgH) {
        QImage img = m_pixmap.toImage();
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
        int px = int(imgPos.x() * dpr);
        int py = int(imgPos.y() * dpr);
        if (px >= 0 && px < img.width() && py >= 0 && py < img.height()) {
            QRgb rgb = img.pixel(px, py);
            int r = qRed(rgb), g = qGreen(rgb), b = qBlue(rgb);
            QString hexStr = QStringLiteral("#%1%2%3")
                .arg(r, 2, 16, QChar('0'))
                .arg(g, 2, 16, QChar('0'))
                .arg(b, 2, 16, QChar('0'));
            if (m_hexUpper) hexStr = hexStr.toUpper();
            else hexStr = hexStr.toLower();
            if (m_hideHash) hexStr = hexStr.mid(1);
            QString rgbStr = QStringLiteral("RGB(%1,%2,%3)").arg(r).arg(g).arg(b);
            QString displayText;
            if (m_colorFmt == "HEX") displayText = hexStr;
            else if (m_colorFmt == "RGB") displayText = rgbStr;
            else if (m_colorFmt == "RGB+HEX") displayText = rgbStr + "  " + hexStr;
            else if (m_colorFmt == "HEX+RGB") displayText = hexStr + "  " + rgbStr;
            QFont f("Consolas", 9);
            p.setFont(f);
            QFontMetrics fm(f);
            int infoW = fm.horizontalAdvance(displayText) + 12;
            QRect infoRect(magRect.left(), magRect.bottom() + 2, infoW, 16);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, 200));
            p.drawRoundedRect(infoRect, 3, 3);
            p.setPen(QColor(255, 255, 255));
            p.drawText(infoRect, Qt::AlignCenter, displayText);
        }
    }

    p.setPen(QPen(QColor(26, 174, 255, 220), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(magRect, 4, 4);
}


static const char SP1_MAGIC[4] = { 'S', 'P', '1', 0x01 };
static const quint8 SP1_VERSION = 2;

QString StickerWindow::persistenceDir() {
    QDir base = ResourceHelper::runtime_base_dir();
    QString dir = base.filePath("history/default");
    QDir().mkpath(dir);
    return dir;
}

QStringList StickerWindow::listPersistedStickers() {
    QDir dir(persistenceDir());
    QStringList files = dir.entryList(QStringList() << "*.sp1", QDir::Files);
    return files;
}

bool StickerWindow::saveState(const QString& path) {
    // Save ORIGINAL unscaled pixmap + all transform params
    // So on restore we can reapply scale/flip/opacity from original
    QByteArray pngBytes;
    QBuffer buf(&pngBytes);
    buf.open(QIODevice::WriteOnly);
    m_originalPixmap.toImage().save(&buf, "PNG");
    buf.close();

    QString tmpPath = path + ".tmp";
    if (!ResourceHelper::save_encrypted_png_bytes(pngBytes, tmpPath)) return false;
    QFile tf(tmpPath);
    if (!tf.open(QIODevice::ReadOnly)) return false;
    QByteArray encData = tf.readAll();
    tf.close();
    tf.remove();

    QFile out(path);
    if (!out.open(QIODevice::WriteOnly)) return false;
    QDataStream ds(&out);
    ds.setVersion(QDataStream::Qt_6_0);
    ds.writeRawData(SP1_MAGIC, 4);
    ds << SP1_VERSION;
    // Position
    ds << qint32(frameGeometry().x()) << qint32(frameGeometry().y());
    // Original dimensions (NOT scaled)
    ds << qint32(m_origImgW) << qint32(m_origImgH);
    // Transform params
    ds << qreal(m_scale);
    ds << qreal(m_opacity);
    ds << bool(m_hMirrored) << bool(m_vMirrored);
    // Annotation list
    ds << qint32(m_annotations.size());
    for (const auto& ann : m_annotations) {
        ds << qint32(static_cast<int>(ann.tool));
        ds << QRgb(ann.color.rgba());
        ds << qint32(ann.width);
        ds << ann.start << ann.end;
        ds << qint32(ann.points.size());
        for (const QPoint& pt : ann.points) ds << pt;
        ds << ann.text;
        ds << ann.font.family() << qreal(ann.font.pointSizeF()) << bool(ann.font.bold()) << bool(ann.font.italic());
        ds << ann.pos;
        ds << qint32(ann.blockSize) << qint32(ann.mosaicOpacity) << bool(ann.mosaicFill) << ann.mosaicMode;
        ds << bool(ann.fill) << ann.hlMode;
        ds << qint32(static_cast<int>(ann.arrowStyle));
    }
    // Encrypted original PNG
    ds.writeBytes(encData.constData(), encData.size());
    out.close();
    return true;
}

StickerWindow* StickerWindow::loadFromState(const QString& path, QWidget* parent) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return nullptr;
    QDataStream ds(&f);
    ds.setVersion(QDataStream::Qt_6_0);
    char magic[4];
    ds.readRawData(magic, 4);
    if (memcmp(magic, SP1_MAGIC, 4) != 0) return nullptr;
    quint8 ver; ds >> ver;
    if (ver != SP1_VERSION) return nullptr;

    qint32 x, y, origW, origH; qreal scale;
    ds >> x >> y >> origW >> origH >> scale;

    qreal opacity;
    bool hMirrored, vMirrored;
    ds >> opacity >> hMirrored >> vMirrored;

    qint32 annCount; ds >> annCount;
    QVector<Annotation> annotations;
    for (int i = 0; i < annCount; ++i) {
        Annotation ann;
        qint32 tool, bs, aStyle; QRgb color; qint32 width;
        bool b;
        ds >> tool >> color >> width;
        ann.tool = static_cast<Tool>(tool);
        ann.color = QColor(color);
        ann.width = width;
        ds >> ann.start >> ann.end;
        qint32 ptCount; ds >> ptCount;
        for (int j = 0; j < ptCount; ++j) {
            QPoint pt; ds >> pt; ann.points.append(pt);
        }
        ds >> ann.text;
        QString family; qreal ps; bool bold, italic;
        ds >> family >> ps >> bold >> italic;
        QFont fnt(family);
        fnt.setPointSizeF(ps); fnt.setBold(bold); fnt.setItalic(italic);
        ann.font = fnt;
        ds >> ann.pos;
        qint32 mo;
        ds >> bs >> mo >> b >> ann.mosaicMode;
        ann.blockSize = bs; ann.mosaicOpacity = mo; ann.mosaicFill = b;
        ds >> b >> ann.hlMode;
        ann.fill = b;
        ds >> aStyle;
        ann.arrowStyle = static_cast<ArrowStyle>(aStyle);
        annotations.append(ann);
    }

    char* encData = nullptr; uint encSize = 0;
    ds.readBytes(encData, encSize);
    QByteArray enc = QByteArray::fromRawData(encData, encSize);
    QString tmpPath = path + ".restore.tmp";
    QFile tmp(tmpPath);
    if (tmp.open(QIODevice::WriteOnly)) { tmp.write(enc); tmp.close(); }
    QByteArray pngBytes = ResourceHelper::load_encrypted_png_bytes(tmpPath);
    QFile::remove(tmpPath);
    delete[] encData;

    QImage img;
    if (!img.loadFromData(pngBytes, "PNG")) return nullptr;

    // This is the ORIGINAL unscaled image (physical pixels)
    QPixmap origPix = QPixmap::fromImage(img);
    qreal dpr = QApplication::primaryScreen()->devicePixelRatio();
    if (dpr <= 0) dpr = 1.0;
    origPix.setDevicePixelRatio(dpr);

    // Recalculate logical dimensions from PHYSICAL pixels, not saved origW/origH.
    // This ensures correct display even if screen DPR changed since last save.
    int physW = origPix.width();   // original physical pixels
    int physH = origPix.height();
    int newOrigW = qMax(1, qRound(physW / dpr));  // logical pixels for current DPR
    int newOrigH = qMax(1, qRound(physH / dpr));

    // Create sticker with original pixmap at scale 1.0 first
    auto* sw = new StickerWindow(origPix, parent);
    // Use recalculated logical dimensions (not saved origW/origH which may be from different DPR)
    sw->m_origImgW = newOrigW;
    sw->m_origImgH = newOrigH;
    // Apply saved transform params
    // Use saved scale directly. The saved PNG is the original full-resolution image,
    // so scale has the same meaning on restore. Recalculating scale based on
    // saved origW vs current physW/dpr can produce wrong results if the saved
    // PNG dimensions dont match saved origW (e.g. from older bakeAnnotations).
    sw->m_scale = qBound(0.2, scale, 5.0);
    sw->m_opacity = opacity;
    sw->m_hMirrored = hMirrored;
    sw->m_vMirrored = vMirrored;
    sw->m_annotations = annotations;

    // Generate display pixmap at correct physical size: origPhys * scale
    int dispPhysW = qMax(1, qRound(physW * sw->m_scale));
    int dispPhysH = qMax(1, qRound(physH * sw->m_scale));
    sw->m_imgW = qMax(1, qRound(dispPhysW / dpr));  // logical display size
    sw->m_imgH = qMax(1, qRound(dispPhysH / dpr));
    sw->m_pixmap = origPix.scaled(dispPhysW, dispPhysH,
                                   Qt::IgnoreAspectRatio,
                                   sw->m_smoothScaling ? Qt::SmoothTransformation : Qt::FastTransformation);
    sw->m_pixmap.setDevicePixelRatio(dpr);
    sw->setFixedSize(sw->m_imgW + SHADOW_MARGIN * 2, sw->m_imgH + SHADOW_MARGIN * 2);
    sw->setWindowOpacity(opacity);
    sw->markCacheDirty();
    sw->move(x, y);
    sw->repositionToolbar();
    sw->update();
    return sw;
}


QIcon StickerWindow::getCheckIcon() {
    
    static const char* checkSvg =
        "<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16'>"
        "<rect width='16' height='16' rx='3' fill='#1AAEFF'/>"
        "<path d='M3 8 L6.5 11.5 L13 4.5' stroke='white' stroke-width='2' fill='none' stroke-linecap='round' stroke-linejoin='round'/>"
        "</svg>";
    return QIcon(QPixmap::fromImage(QImage::fromData(QByteArray(checkSvg))));
}

void StickerWindow::rotateImage(bool clockwise) {
    QTransform t;
    t.rotate(clockwise ? 90 : -90);
    QImage img = m_originalPixmap.toImage().transformed(t, Qt::SmoothTransformation);
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    m_originalPixmap = QPixmap::fromImage(img);
    m_originalPixmap.setDevicePixelRatio(dpr);
    m_annotations.clear();
    m_redoStack.clear();
    std::swap(m_hMirrored, m_vMirrored);
    std::swap(m_origImgW, m_origImgH);
    m_imgW = qMax(1, int(m_origImgW * m_scale));
    m_imgH = qMax(1, int(m_origImgH * m_scale));
    m_pixmap = m_originalPixmap.scaled(qRound(m_imgW * dpr), qRound(m_imgH * dpr),
                                       Qt::IgnoreAspectRatio,
                                       m_smoothScaling ? Qt::SmoothTransformation : Qt::FastTransformation);
    m_pixmap.setDevicePixelRatio(dpr);
    setFixedSize(m_imgW + SHADOW_MARGIN * 2, m_imgH + SHADOW_MARGIN * 2);
    markCacheDirty();
    repositionToolbar();
    update();
    saveSnipFile();
    saveStickerState();
}

void StickerWindow::flipImage(bool horizontal) {
    if (horizontal) m_hMirrored = !m_hMirrored;
    else m_vMirrored = !m_vMirrored;
    QImage img = m_originalPixmap.toImage().mirrored(horizontal, !horizontal);
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    m_originalPixmap = QPixmap::fromImage(img);
    m_originalPixmap.setDevicePixelRatio(dpr);
    m_annotations.clear();
    m_redoStack.clear();
    m_imgW = qMax(1, int(m_origImgW * m_scale));
    m_imgH = qMax(1, int(m_origImgH * m_scale));
    m_pixmap = m_originalPixmap.scaled(qRound(m_imgW * dpr), qRound(m_imgH * dpr),
                                       Qt::IgnoreAspectRatio,
                                       m_smoothScaling ? Qt::SmoothTransformation : Qt::FastTransformation);
    m_pixmap.setDevicePixelRatio(dpr);
    setFixedSize(m_imgW + SHADOW_MARGIN * 2, m_imgH + SHADOW_MARGIN * 2);
    markCacheDirty();
    repositionToolbar();
    update();
    saveSnipFile();
    saveStickerState();
}

void StickerWindow::zoomTo(qreal scale) {
    m_scale = qBound(0.2, scale, 5.0);
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    int newW = qMax(1, int(m_origImgW * m_scale));
    int newH = qMax(1, int(m_origImgH * m_scale));
    m_pixmap = m_originalPixmap.scaled(qRound(newW * dpr), qRound(newH * dpr),
                                       Qt::IgnoreAspectRatio,
                                       m_smoothScaling ? Qt::SmoothTransformation : Qt::FastTransformation);
    m_pixmap.setDevicePixelRatio(dpr);
    m_imgW = newW;
    m_imgH = newH;
    setFixedSize(m_imgW + SHADOW_MARGIN * 2, m_imgH + SHADOW_MARGIN * 2);
    markCacheDirty();
    repositionToolbar();
    update();
    showToast(QString("%1%").arg(int(m_scale * 100)), 800);
    saveStickerState();
}

void StickerWindow::toggleSmooth() {
    m_smoothScaling = !m_smoothScaling;
    update();
}

void StickerWindow::toggleShadow(bool checked) {
    m_shadowEnabled = checked;
    update();
}

void StickerWindow::toggleToolbar(bool checked) {
    m_showToolbar = checked;
    if (checked) setSelected(true);
    updateToolbar();
    update();
}

QPixmap StickerWindow::getRenderedPixmap() const {
    if (m_annotations.isEmpty()) return m_pixmap;
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    int w = m_pixmap.width();
    int h = m_pixmap.height();
    if (w <= 0 || h <= 0) return m_pixmap;
    QPixmap result(w, h);
    if (result.isNull()) return m_pixmap;
    result.fill(Qt::transparent);
    QPainter p(&result);
    if (!p.isActive()) return m_pixmap;
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPixmap(QRect(0, 0, w, h), m_pixmap);
    for (const auto& ann : m_annotations) {
        const_cast<Annotation&>(ann).selected = false;
        const_cast<StickerWindow*>(this)->drawAnnotation(p, ann);
    }
    p.end();
    result.setDevicePixelRatio(dpr);
    return result;
}

QPixmap StickerWindow::getOriginalRenderedPixmap() const {
    if (m_originalPixmap.isNull()) return getRenderedPixmap();
    if (m_annotations.isEmpty()) return m_originalPixmap;
    qreal dpr = qApp->primaryScreen() ? qApp->primaryScreen()->devicePixelRatio() : 1.0;
    int w = m_originalPixmap.width();
    int h = m_originalPixmap.height();
    if (w <= 0 || h <= 0) return m_originalPixmap;
    QPixmap result(w, h);
    if (result.isNull()) return m_originalPixmap;
    result.fill(Qt::transparent);
    QPainter p(&result);
    if (!p.isActive()) return m_originalPixmap;
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPixmap(QRect(0, 0, w, h), m_originalPixmap);
    
    qreal sx = qreal(w) / qMax(1, m_pixmap.width());
    qreal sy = qreal(h) / qMax(1, m_pixmap.height());
    for (auto ann : m_annotations) {
        ann.start = QPoint(int(ann.start.x() * sx), int(ann.start.y() * sy));
        ann.end = QPoint(int(ann.end.x() * sx), int(ann.end.y() * sy));
        for (auto& pt : ann.points) pt = QPoint(int(pt.x() * sx), int(pt.y() * sy));
        ann.pos = QPoint(int(ann.pos.x() * sx), int(ann.pos.y() * sy));
        ann.selected = false;
        const_cast<StickerWindow*>(this)->drawAnnotation(p, ann);
    }
    p.end();
    result.setDevicePixelRatio(dpr);
    return result;
}

void StickerWindow::copyOriginalToClipboard() {
    QPixmap pix = getOriginalRenderedPixmap();
    QApplication::clipboard()->setPixmap(pix);
    showToast(I18n::t("msg.copied_short", QStringLiteral("已复制")), 1500);
}

void StickerWindow::saveOriginalToFile() {
    QString path = QFileDialog::getSaveFileName(this,
        I18n::t("capture.save_title", QStringLiteral("保存图片")),
        QString(), "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)");
    if (!path.isEmpty()) {
        getOriginalRenderedPixmap().save(path);
        showToast(I18n::t("msg.saved", QStringLiteral("已保存")), 1500);
    }
}

void StickerWindow::pasteFromClipboard() {
    QPixmap pix = QApplication::clipboard()->pixmap();
    if (pix.isNull()) {
        showToast(I18n::t("msg.clipboard_empty", QStringLiteral("剪贴板无图片")), 1500);
        return;
    }
    qreal dpr = pix.devicePixelRatio() > 0 ? pix.devicePixelRatio() : 1.0;
    m_originalPixmap = pix;
    m_originalPixmap.setDevicePixelRatio(dpr);
    m_annotations.clear();
    m_redoStack.clear();
    m_scale = 1.0;
    m_imgW = int(pix.width() / dpr);
    m_imgH = int(pix.height() / dpr);
    m_origImgW = m_imgW;
    m_origImgH = m_imgH;
    m_pixmap = m_originalPixmap;
    setFixedSize(m_imgW + SHADOW_MARGIN * 2, m_imgH + SHADOW_MARGIN * 2);
    markCacheDirty();
    repositionToolbar();
    update();
    saveStickerState();
}

void StickerWindow::replaceWithFile() {
    QString path = QFileDialog::getOpenFileName(this,
        I18n::t("sticker.replace_with_file", QStringLiteral("替换为文件...")),
        QString(), "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp)");
    if (path.isEmpty()) return;
    QImage img(path);
    if (img.isNull()) {
        showToast(I18n::t("msg.load_failed", QStringLiteral("加载失败")), 1500);
        return;
    }
    QPixmap pix = QPixmap::fromImage(img);
    qreal dpr = pix.devicePixelRatio() > 0 ? pix.devicePixelRatio() : 1.0;
    m_originalPixmap = pix;
    m_originalPixmap.setDevicePixelRatio(dpr);
    m_annotations.clear();
    m_redoStack.clear();
    m_scale = 1.0;
    m_imgW = pix.width();
    m_imgH = pix.height();
    m_origImgW = m_imgW;
    m_origImgH = m_imgH;
    m_pixmap = m_originalPixmap;
    setFixedSize(m_imgW + SHADOW_MARGIN * 2, m_imgH + SHADOW_MARGIN * 2);
    markCacheDirty();
    repositionToolbar();
    update();
    saveStickerState();
}

void StickerWindow::viewInFolder() {
    
    QString filePath = m_originalFilePath.isEmpty() ? m_historyFilePath : m_originalFilePath;
    if (filePath.isEmpty()) return;
    std::wstring wpath = QDir::toNativeSeparators(filePath).toStdWString();
    LPITEMIDLIST pidl = ILCreateFromPathW(wpath.c_str());
    if (pidl) {
        SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
        ILFree(pidl);
    } else {
        // fallback
        QProcess::startDetached(QStringLiteral("explorer /select,\"%1\"").arg(QDir::toNativeSeparators(filePath)));
    }
}

void StickerWindow::destroySticker() {
    // Python _destroy: delete snip copy, original file, and index entries, then close.
    QString snipName;
    if (!m_historyFilePath.isEmpty()) {
        snipName = QFileInfo(m_historyFilePath).fileName();
    }
    for (const QString& fpath : {m_historyFilePath, m_originalFilePath}) {
        if (!fpath.isEmpty() && QFile::exists(fpath)) {
            QFile::remove(fpath);
        }
    }
    if (!snipName.isEmpty()) {
        ResourceHelper::remove_snip_entry(snipName);
    }
    removeStickerState();
    close();
}

void StickerWindow::showToast(const QString& text, int duration) {
    m_toastText = text;
    m_toastTimer->start(duration);
    update();
}

void StickerWindow::hideToast() {
    m_toastText.clear();
    update();
}

bool StickerWindow::saveSnipFile() {
    
    if (m_historyFilePath.isEmpty()) return false;
    return saveState(m_historyFilePath);
}

void StickerWindow::saveStickerState() {
    if (m_historyFilePath.isEmpty()) return;
    saveSnipFile();
    QFileInfo fi(m_historyFilePath);
    QString snipName = fi.fileName();
    QPoint p = pos();
    ResourceHelper::set_sticker_state(snipName, p.x(), p.y(), m_scale,
                                       m_origImgW, m_origImgH, m_opacity,
                                       m_hMirrored, m_vMirrored);
}

void StickerWindow::removeStickerState() {
    if (m_historyFilePath.isEmpty()) return;
    QFileInfo fi(m_historyFilePath);
    QString snipName = fi.fileName();
    ResourceHelper::remove_sticker_state(snipName);
}

void StickerWindow::moveToGroup(const QString& groupId) {
    if (m_originalFilePath.isEmpty()) return;
    QFileInfo srcInfo(m_originalFilePath);
    if (!srcInfo.exists()) return;
    QDir base = ResourceHelper::runtime_base_dir();
    QDir dstDir = base.filePath("history/" + groupId);
    dstDir.mkpath(".");
    QString dstPath = dstDir.filePath(srcInfo.fileName());
    if (QFile::exists(dstPath)) QFile::remove(dstPath);
    if (!QFile::rename(m_originalFilePath, dstPath)) {
        // rename may fail across drives; fall back to copy+delete
        if (QFile::copy(m_originalFilePath, dstPath)) {
            QFile::remove(m_originalFilePath);
        } else {
            showToast(I18n::t("msg.move_failed", QStringLiteral("移动失败")));
            return;
        }
    }
    m_originalFilePath = dstPath;
    // update snip index
    QFileInfo snipInfo(m_historyFilePath);
    QString snipName = snipInfo.fileName();
    QString rel = groupId + "/" + srcInfo.fileName();
    ResourceHelper::set_snip_entry(snipName, rel);
    showToast(I18n::t("msg.moved_to_group", QStringLiteral("已移动到分组")), 1500);
}

void StickerWindow::createGroupAndMove() {
    QDialog dlg(this);
    dlg.setWindowTitle(I18n::t("tray.group_name_title", QStringLiteral("新建分组")));
    auto* lay = new QVBoxLayout(&dlg);
    lay->addWidget(new QLabel(I18n::t("tray.group_name_prompt", QStringLiteral("请输入分组名称：")), &dlg));
    auto* edit = new QLineEdit(I18n::t("tray.group_name_default", QStringLiteral("新分组")), &dlg);
    edit->selectAll();
    lay->addWidget(edit);
    auto* chk = new QCheckBox(I18n::t("tray.group_activate", QStringLiteral("创建后立即激活")), &dlg);
    chk->setChecked(true);
    lay->addWidget(chk);
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    lay->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    // center on screen
    QScreen* screen = QApplication::screenAt(QCursor::pos());
    if (!screen) screen = QApplication::primaryScreen();
    if (screen) {
        QRect sg = screen->availableGeometry();
        dlg.adjustSize();
        dlg.move(sg.center().x() - dlg.width() / 2, sg.center().y() - dlg.height() / 2);
    }
    if (dlg.exec() != QDialog::Accepted) return;
    QString name = edit->text().trimmed();
    if (name.isEmpty()) return;
    // generate unique group id
    QJsonArray groups = qvariant_cast<QJsonArray>(ConfigManager::instance().get("groups"));
    if (groups.isEmpty()) {
            QJsonObject def; def["id"] = "default"; def["name"] = QStringLiteral("默认");
        groups.append(def);
    }
    QSet<QString> existingIds;
    for (const QJsonValue& gv : groups) existingIds.insert(gv.toObject().value("id").toString());
    QString gid;
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    while (true) {
        gid.clear();
        for (int i = 0; i < 6; i++) gid += chars[QRandomGenerator::global()->bounded(chars.size())];
        if (!existingIds.contains(gid)) break;
    }
    QJsonObject ng; ng["id"] = gid; ng["name"] = name;
    groups.append(ng);
    ConfigManager::instance().set("groups", groups);
    if (chk->isChecked()) ConfigManager::instance().set("active_group_id", gid);
    moveToGroup(gid);
}

} // namespace sticker

#include "sticker_window.moc"
