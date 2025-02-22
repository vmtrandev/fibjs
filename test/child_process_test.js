var test = require("test");
test.setup();

var test_util = require('./test_util');

const child_process = require('child_process');
var coroutine = require("coroutine");
var path = require('path');
var json = require('json');
var ws = require('ws');
var net = require('net');
var http = require('http');
var io = require('io');
var os = require('os');

var envKeys = require('./process/const.env_keys.js');

describe("child_process", () => {
    var cmd;

    before(() => {
        cmd = process.execPath;
    });

    after(test_util.cleanup);

    it("stdout", () => {
        var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.js')]);
        var stdout = new io.BufferedStream(bs.stdout);

        assert.equal(stdout.readLine(), "exec testing....");

        var t0 = new Date().getTime();

        stdout.readLine();
        assert.equal(stdout.readLine(), "console.print....");
        assert.closeTo(new Date().getTime() - t0, 1000, 500);

        stdout.readLine();
        assert.equal(stdout.readLine(), "console.print....");
        assert.closeTo(new Date().getTime() - t0, 2000, 500);

        var stderr = new io.BufferedStream(bs.stderr);
        assert.deepEqual(stderr.readLines(), [
            "warn exec testing....",
            "error exec testing...."
        ]);
    });

    describe("ChildProcess::std[xx].fd", () => {
        it("ChildProcess::stdin.fd", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.js')]);
            var stdin = new io.BufferedStream(bs.stdin);

            assert.isDefined(stdin.fd);
            assert.ok(stdin.fd > 2 || stdin.fd < 0);
        });

        it("ChildProcess::stdout.fd", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.js')]);
            var stdout = new io.BufferedStream(bs.stdout);

            assert.isDefined(stdout.fd);
            assert.ok(stdout.fd > 2 || stdout.fd < 0);
        });

        it("ChildProcess::stderr.fd", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.js')]);
            var stderr = new io.BufferedStream(bs.stderr);

            assert.isDefined(stderr.fd);
            assert.ok(stderr.fd > 2 || stderr.fd < 0);
        });
    });

    describe("stdout/stderr", () => {
        it("SubProcess::stderr/stdout exist", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.js')]);

            assert.exist(bs.stdout);
            assert.exist(bs.stderr);
        });

        it("stdout output", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.stdout.js')]);
            var stdout = new io.BufferedStream(bs.stdout);

            assert.equal(stdout.readLine(), "exec testing....");

            var t0 = new Date().getTime();

            stdout.readLine();
            var offsets = []
            offsets[0] = new Date().getTime() - t0;
            assert.closeTo(offsets[0], 1000, 500);

            stdout.readLine();
            offsets[1] = new Date().getTime() - t0;
            assert.closeTo(offsets[1], 2000, 1000);
        });

        it("stderr output", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.stderr.js')]);
            var stderr = new io.BufferedStream(bs.stderr);

            assert.equal(stderr.readLine(), "exec testing....");

            var t0 = new Date().getTime();

            stderr.readLine();
            var offsets = []
            offsets[0] = new Date().getTime() - t0;
            assert.closeTo(offsets[0], 1000, 500);

            stderr.readLine();
            offsets[1] = new Date().getTime() - t0;
            assert.closeTo(offsets[1], 2000, 1000);
        });

        if (process.platform != "win32")
            it("pty output", () => {
                var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.stdout.js')], {
                    stdio: 'pty'
                });
                var stdout = new io.BufferedStream(bs.stdout);

                assert.equal(stdout.readLine(), "exec testing....\r");

                var t0 = new Date().getTime();

                stdout.readLine();
                var offsets = []
                offsets[0] = new Date().getTime() - t0;
                assert.closeTo(offsets[0], 1000, 500);

                stdout.readLine();
                offsets[1] = new Date().getTime() - t0;
                assert.closeTo(offsets[1], 2000, 1000);
            });

        it("console stdout output", () => {
            var status = child_process.run(cmd, [path.join(__dirname, 'process', 'exec.stdout.js')]);
            assert.equal(status, 0);
        });

        it("parallel stdin", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.parallel_stdin.js')]);
            var str = "";
            coroutine.sleep(100);
            for (var i = 0; i < 10; i++) {
                coroutine.sleep(10);
                var id = test_util.makeid(10);
                str += id;
                bs.stdin.write(id);
            }
            var str1 = bs.stdout.read(100).toString();
            assert.equal(str, str1);
        })
    });

    it("stdin/stdout", () => {
        var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec1.js')]);
        var stdout = new io.BufferedStream(bs.stdout);

        bs.stdin.write("hello, exec1" + os.EOL);
        assert.equal(stdout.readLine(), "hello, exec1");
    });

    it("fork", () => {
        var bs = child_process.fork(path.join(__dirname, 'process', 'exec1.js'), {
            stdio: "pipe"
        });
        var stdout = new io.BufferedStream(bs.stdout);

        bs.stdin.write("hello, exec1" + os.EOL);
        assert.equal(stdout.readLine(), "hello, exec1");
    });

    xit("stdin/stdout stream", () => {
        var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.chargeable.js')]);
        var stdout = new io.BufferedStream(bs.stdout);
        var outputs = []

        process.nextTick(() => {
            var oline = null

            while (true) {
                oline = stdout.readLine()
                if (oline === 'exit') {
                    break
                }

                if (oline) {
                    outputs.push(oline)
                }
            }
        });

        process.nextTick(() => {
            bs.stdin.write('line1' + os.EOL)
            bs.stdin.write('line2' + os.EOL)
            bs.stdin.write('.exit' + os.EOL)
        })

        bs.join()

        assert.deepEqual(
            outputs,
            [
                `> your input is: line1`,
                `> your input is: line2`,
            ]
        )
    });

    it("run", () => {
        assert.equal(child_process.run(cmd, [path.join(__dirname, 'process', 'exec.js')]), 100);
    });

    it("exitCode", () => {
        assert.equal(child_process.run(cmd, [path.join(__dirname, 'process', 'exec13.js')]), 100);
        assert.equal(child_process.run(cmd, [path.join(__dirname, 'process', 'exec14.js')]), 101);
    });

    it("run throw error", () => {
        assert.throws(() => {
            child_process.run("not_exists_exec_file");
        });
    });

    it("multi run", () => {
        coroutine.parallel([1, 2, 3, 4, 5, 6], (n) => {
            assert.equal(child_process.run(cmd, [path.join(__dirname, 'process', 'exec6.js'), n]), n);
        });
    });

    it("FIX: crash when bad stdio array", () => {
        child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.js')], {
            stdio: ['pipe', ,]
        });
    });

    describe('process holding', () => {
        it("multi fiber", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec7.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "100");
            p.join();
            assert.equal(p.exitCode, 7);
        });

        it("pendding callback", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec8.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "200");
            p.join();
            assert.equal(p.exitCode, 8);
        });

        it("setTimeout", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec9.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "300");
            p.join();
            assert.equal(p.exitCode, 9);
        });

        it("setTimeout unref", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec9.1.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "301");
            p.join();
            assert.equal(p.exitCode, 0);
        });

        it("setTimeout ref", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec9.2.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "302");
            p.join();
            assert.equal(p.exitCode, 9);
        });

        it("setInterval", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec10.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "400");
            p.join();
            assert.equal(p.exitCode, 10);
        });

        it("setImmediate", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec11.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "500");
            p.join();
            assert.equal(p.exitCode, 11);
        });

        it("websocket connect", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec18.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            p.join();
            assert.equal(p.exitCode, 81);
        });

        it("websocket disconnect", () => {
            var httpd = new http.Server(8899, {
                "/ws": ws.upgrade((s) => {
                    s.onmessage = function (msg) {
                        s.send(msg);
                    };
                })
            });
            test_util.push(httpd.socket);
            httpd.start();

            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec19.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "1900");
            p.join();
            assert.equal(p.exitCode, 19);
        });

        it("worker", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec20.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "2000");
            p.join();
            assert.equal(p.exitCode, 20);
        });

        it("bugfix: multi fiber async", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec12.js')]);
            var stdout = new io.BufferedStream(p.stdout);
            assert.equal(stdout.readLine(), "600");
            p.join();
            assert.equal(p.exitCode, 12);
        });

        it("tcp server", () => {
            var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec21.js')]);
            var stdout = new io.BufferedStream(p.stdout);

            for (var i = 0; i < 100; i++) {
                coroutine.sleep(500);
                try {
                    net.connect('tcp://127.0.0.1:28080');
                    break;
                } catch (e) { }
            }

            assert.equal(stdout.readLine(), "700");
            p.join();
            assert.equal(p.exitCode, 21);
        });
    });

    it("spawn", () => {
        var t1 = new Date().getTime();
        child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.js')], {
            stdio: 'inherit'
        });
        assert.lessThan(new Date().getTime() - t1, 100);
    });

    it("kill", () => {
        var t1 = new Date().getTime();
        var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec.js')], {
            stdio: 'inherit'
        });
        coroutine.sleep(500);
        p.kill(15);
        p.join();
        assert.lessThan(new Date().getTime() - t1, 1000);
    });

    it("usage", () => {
        var p = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec22.js')]);
        var o = JSON.parse(p.stdout.read().toString());
        var o1 = p.usage();

        try {
            assert.closeTo(o.user, o1.user, 50000);
            assert.closeTo(o.system, o1.system, 50000);

            if (o.rss > 0)
                assert.closeTo(o.rss, o1.rss, o.rss / 2);
        } finally {
            p.kill(15);
            p.join();
        }
    });

    it("argv", () => {
        assert.deepEqual(json.decode(child_process.execFile(cmd, [
            path.join(__dirname, "process", "exec2.js"),
            "arg1",
            "arg2"
        ]).stdout), [
            cmd, path.join(__dirname, "process", "exec2.js"), "arg1", "arg2"
        ]);
    });

    it("argv 1", () => {
        assert.deepEqual(json.decode(child_process.execFile(cmd, [
            "--use_strict",
            "--test1",
            path.join(__dirname, "process", "exec2.js"),
            "arg1",
            "arg2"
        ]).stdout), [
            cmd, path.join(__dirname, "process", "exec2.js"), "arg1", "arg2"
        ]);
    });

    it("argv utf8", () => {
        assert.deepEqual(json.decode(child_process.execFile(cmd, [
            path.join(__dirname, "process", "exec2.js"),
            "参数1",
            "参数2"
        ]).stdout), [
            cmd, path.join(__dirname, "process", "exec2.js"), "参数1", "参数2"
        ]);
    });

    it("execArgv", () => {
        assert.deepEqual(json.decode(child_process.execFile(cmd, [
            "--use_strict",
            "--test",
            path.join(__dirname, "process", "exec3.js"),
            "arg1",
            "arg2"
        ]).stdout), [
            "--use_strict",
            "--test",
        ]);
    });

    it("env", () => {
        process.env.abc = 123;

        assert.equal(json.decode(child_process.execFile(cmd, [
            path.join(__dirname, "process", "exec4.js")
        ]).stdout).abc, "123");
    });

    it("env1", () => {
        var env = json.decode(child_process.execFile(cmd, [
            path.join(__dirname, "process", "exec4.js")
        ], {
            env: {
                QEMU_LD_PREFIX: process.env.QEMU_LD_PREFIX,
                abcd: "234"
            }
        }).stdout);

        assert.isUndefined(env.abc);
        assert.equal(env.abcd, "234");
    });

    if (process.platform != "win32") {
        xit("PATH env", () => {
            assert.equal(child_process.run("ls", [path.join(__dirname, "process")]), 0)
            assert.ok(child_process.execFile("ls", ["-a", path.join(__dirname, "process")]).stdout);
        });

        it("umask()", () => {
            const mask = '0664';
            const unmask = process.umask();
            // assert.equal(0o777 & ~unmask, 0o755);

            const old = process.umask(mask);
            assert.equal(parseInt(mask, 8), process.umask(old));

            // confirm reading the umask does not modify it.
            // 1. If the test fails, this call will succeed, but the mask will be set to 0
            assert.equal(old, process.umask());
            // 2. If the test fails, process.umask() will return 0
            assert.equal(old, process.umask());
        });
    }

    describe("Event", () => {
        it("beforeExit", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec15.js')]);
            var stdout = new io.BufferedStream(bs.stdout);
            assert.deepEqual(stdout.readLines(), [
                "beforeExit 101",
                "other beforeExit 101",
                "new work 101",
                "beforeExit 101",
                "other beforeExit 101"
            ]);

            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec16.js')]);
            var stdout = new io.BufferedStream(bs.stdout);
            assert.deepEqual(stdout.readLines(), []);
        });

        it("exit", () => {
            var bs = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec17.js')]);
            var stdout = new io.BufferedStream(bs.stdout);
            assert.deepEqual(stdout.readLines(), [
                "exit 101",
                "other exit 101"
            ]);
        });
    });

    describe("ipc", () => {
        it("init variable", () => {
            var cp = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec25.js')], {
                "stdio": ['inherit', 'inherit', 'inherit']
            });
            cp.join();
            assert.equal(cp.exitCode, 1);
        });

        it("replace stdio", () => {
            var cp = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec25.js')], {
                "stdio": ['ipc', 'inherit', 'inherit']
            });
            assert.equal(cp.stdin, null);

            var cp = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec25.js')], {
                "stdio": ['inherit', 'ipc', 'inherit']
            });
            assert.equal(cp.stdout, null);

            var cp = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec25.js')], {
                "stdio": ['inherit', 'inherit', 'ipc']
            });
            assert.equal(cp.stderr, null);
        });

        it("can have only one IPC pipe", () => {
            assert.throws(() => {
                var n = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec1.js')], {
                    "stdio": ['ipc', 'ipc', 'inherit']
                });
            });
        });

        it("hold process on message", () => {
            var p = child_process.fork(path.join(__dirname, 'process', 'exec23.js'));
            setTimeout(() => {
                p.send(1);
            }, 1);
            p.join();
            assert.equal(p.exitCode, 12);
        });

        it("send message", () => {
            var k;
            var p = child_process.fork(path.join(__dirname, 'process', 'exec24.js'));
            p.on("message", m => {
                if (m == 100)
                    k = true;
            });

            p.send(100);

            for (var i = 0; i < 10000 && !k; i++)
                coroutine.sleep(1);

            assert.equal(k, true);
        });

        it("disconnect", () => {
            var cp = child_process.spawn(cmd, [path.join(__dirname, 'process', 'exec25.1.js')], {
                "stdio": ['ipc', 'inherit', 'inherit']
            });

            assert.equal(cp.connected, true);

            cp.send(100);

            cp.join();

            for (var i = 0; i < 100 && cp.connected; i++)
                coroutine.sleep(1);

            assert.equal(cp.connected, false);
            assert.equal(cp.exitCode, 1);
        });

        it("grandson process", () => {
            var k;
            var cp = child_process.fork(path.join(__dirname, 'process', 'exec27.js'));
            cp.join();

            assert.equal(cp.exitCode, 1);
        });
    });

    // leave here to tuning manually
    xit("print child process's env items", () => {
        var retcode = child_process.run(cmd, [path.join(__dirname, 'process', 'exec.print_kvs.js')]);
        assert.equal(retcode, 0)
    });
});

require.main === module && test.run(console.DEBUG);