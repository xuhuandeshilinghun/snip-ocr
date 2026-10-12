#include "region_router.h"
#include "sys_env_helper.h"

namespace RegionRouter {


static const QString GITEE_BASE = "https://gitee.com/edenl/snap-text-update";
static const QString GITEE_ISSUES = GITEE_BASE + "/issues";
static const QString GITEE_DOWNLOAD = GITEE_BASE + "/releases/download";
static const QString GITEE_VERSION_JSON = GITEE_BASE + "/raw/master/version.json";


static const QString GITHUB_BASE = "https://github.com/xuhuandeshilinghun/snap-text-update";
static const QString GITHUB_ISSUES = GITHUB_BASE + "/issues";
static const QString GITHUB_DOWNLOAD = GITHUB_BASE + "/releases/download";
static const QString GITHUB_VERSION_JSON = "https://raw.githubusercontent.com/xuhuandeshilinghun/snap-text-update/master/version.json";

// Always return both URLs; caller probes in region-based order
QStringList get_version_urls() {
    return { GITEE_VERSION_JSON, GITHUB_VERSION_JSON };
}

bool is_china() {
    return SysEnvHelper::is_china_locale();
}

QString get_issues_url() {
    if (is_china()) {
        return GITEE_ISSUES;
    }
    return GITHUB_ISSUES;
}

QString get_download_base_url() {
    if (is_china()) {
        return GITEE_DOWNLOAD;
    }
    return GITHUB_DOWNLOAD;
}

QString build_download_url(const QString& version, const QString& filename) {
    
    QString ver = version;
    if (ver.startsWith('V') || ver.startsWith('v'))
        ver = ver.mid(1);
    return get_download_base_url() + "/v" + ver + "/" + filename;
}

} // namespace RegionRouter
