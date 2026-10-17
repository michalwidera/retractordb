"""Kontrole strukturalne Etapu 12; nie sa dowodem zachowania galezi niewykonanej."""

from pathlib import Path
import re
import sys
import unittest


ROOT = Path(sys.argv.pop(1)).resolve()
HEADERS = (
    "src/include/osPlatform.hpp", "src/include/uxSysTermTools.hpp", "src/qry/formatters.hpp",
    "src/qry/qry.hpp", "src/rdb/ICommand.hpp", "src/rdb/xtrdbStorageMap.hpp",
    "src/retractor/lib/appConfig.hpp", "src/retractor/lib/executor_rt.hpp",
    "src/retractor/lib/presenter.hpp", "src/retractor/lib/serverName.hpp",
    "src/retractor/lib/serviceControl.hpp", "src/retractor/lib/shmBudget.hpp",
)


def code(path):
    # Zachowuje literaly: znaczenie maja tu nazwy plikow, a nie komentarze.
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')
    return tokens.sub(lambda match: "" if match[0].startswith(("//", "/*")) else match[0],
                      (ROOT / path).read_text())


class Stage12Structure(unittest.TestCase):
    def test_names_have_finite_catalogues_and_device_seed(self):
        # covers: srvname.generate.shape~1, srvname.generate.device-seed~1, srvname.generate.no-uniqueness~1
        source = code("src/retractor/lib/serverName.cpp")
        for name in ("kAdjectives", "kSurnames"):
            match = re.search(r"std::array<std::string_view,\s*64>\s+" + name + r"\{([^}]+)\}", source)
            self.assertIsNotNone(match, name)
            entries = re.findall(r'"([a-z]+)"', match[1])
            self.assertEqual(len(entries), 64)
            self.assertEqual(len(set(entries)), 64)
        generate = source.split("std::string generate()", 1)[1].split("std::string environmentNamespace()", 1)[0]
        self.assertRegex(generate, r"std::random_device\s+rd\s*;")
        self.assertRegex(generate, r"std::mt19937\s+gen\(rd\(\)\)")
        self.assertNotRegex(generate, r"\b(?:time|clock|system_clock|steady_clock)\b")

    def test_configuration_discovery_orders_layers_and_records_after_validation(self):
        source = code("src/retractor/lib/appConfig.cpp")
        load = source.split("AppConfig loadAppConfig", 1)[1]
        paths = [load.index(path) for path in (
            "/etc/retractor/retractor.toml", "/usr/local/etc/retractor/retractor.toml",
            "/opt/homebrew/etc/retractor/retractor.toml", "if (auto up = userConfigPath()",
        )]
        self.assertEqual(paths, sorted(paths))
        self.assertRegex(load, r"if \(cliPath\)\s*\{[\s\S]*?loadLayer\(\*cliPath\)[\s\S]*?return cfg;")
        self.assertLess(load.index("applyTable(tbl, cfg, path)"), load.index("cfg.loadedFrom.push_back(path)"))
        self.assertRegex(load, r"for \(const auto &path : candidates\)[\s\S]*?loadLayer\(path.string\(\)\)")
        user = source.split("userConfigPath()", 1)[1].split("}  \n", 1)[0]
        self.assertLess(user.index('std::getenv("XDG_CONFIG_HOME")'), user.index('std::getenv("HOME")'))
        self.assertIn('".config" / "retractor" / "retractor.toml"', user)

    def test_platform_identity_dispatch_and_service_filters(self):
        source = code("src/common/osPlatform.cpp")
        dispatch = source.split("ServiceIdentity detectServiceIdentity()", 1)[1]
        self.assertRegex(dispatch, r"#if RDB_HAS_SYSTEMD\s+return detectSystemd\(\)")
        self.assertRegex(dispatch, r"#elif RDB_HAS_LAUNCHD\s+return detectLaunchd\(\)")
        self.assertRegex(dispatch, r"#else\s+return \{\}")
        self.assertIn('seg.ends_with(".service") && !seg.starts_with("user@")', source)
        self.assertIn('path.find("/user.slice")', source)
        self.assertIn('path.find("/user@")', source)
        self.assertIn('name.empty() || name == "0"', source)
        self.assertIn('name.starts_with("application.")', source)
        self.assertIn("getppid() != 1", source)
        self.assertIn("retVal.userScope = (geteuid() != 0)", source)

    def test_client_clamps_budgets_and_joins_before_the_verdict(self):
        source = code("src/qry/qry.cpp")
        self.assertIn("serverNoDataTimeoutMs_(std::max(1, serverNoDataTimeoutMs))", source)
        self.assertIn("std::make_unique<IpcClient>(clientResponseMaxFails, responseQueueOpenMaxFails, serverName)", source)
        select = source.split("selectResult qry::select(", 1)[1]
        self.assertLess(select.rindex("transport_->done = true;", 0, select.index("producer_thread.join()")),
                        select.index("producer_thread.join()"))
        self.assertLess(select.index("producer_thread.join()"), select.index("if (loopFailed)"))
        self.assertIn('const bool ignoreAnyKey = vm.contains("needctrlc") || iElemLimit > 0;', select)

    def test_every_stage_requirement_has_a_qualification_and_gtest_links_exist(self):
        definitions = set()
        for path in (ROOT / "test/UnitTest").glob("*.cpp"):
            definitions.update(".".join(parts) for parts in re.findall(
                r"\bTEST(?:_F|_P)?\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)", path.read_text()))
        for header in HEADERS:
            text = (ROOT / header).read_text()
            for match in re.finditer(r"^/// - .*\n(?:^///(?! - ).*\n)*", text, re.M):
                with self.subTest(header=header, requirement=match[0].splitlines()[0]):
                    # Punkt z ID kwalifikuje st_req_trace (scripts/req-trace.py), nie ta kontrola.
                    self.assertRegex(match[0], r"Test:|Sprawdzenie kompilacji:|Nietestowalne:|^/// - \[[^\]\s]+~\d+\] ")
                    for line in match[0].splitlines():
                        if "Test:" not in line:
                            continue
                        for link in re.findall(r"`(\w+\.\w+)`", line):
                            self.assertIn(link, definitions)


if __name__ == "__main__":
    unittest.main()
