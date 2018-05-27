"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __generator = (this && this.__generator) || function (thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g;
    return g = { next: verb(0), "throw": verb(1), "return": verb(2) }, typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (_) try {
            if (f = 1, y && (t = y[op[0] & 2 ? "return" : op[0] ? "throw" : "next"]) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [0, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
};
var _this = this;
Object.defineProperty(exports, "__esModule", { value: true });
var child_process = require("child_process");
var file_manager = require("./FileManager");
var socket_manager = require("./SocketManager");
var processes = require("./IDEProcesses");
var paths = require("./paths");
var Lock_1 = require("./Lock");
var cpu_monitor = require("./CPUMonitor");
var lock = new Lock_1.Lock();
// this function gets called whenever the ace editor is modified
// the file data is saved robustly using a lockfile, and a syntax
// check started if the flag is set
function upload(data) {
    return __awaiter(this, void 0, void 0, function () {
        var e_1;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, lock.acquire()];
                case 1:
                    _a.sent();
                    _a.label = 2;
                case 2:
                    _a.trys.push([2, 4, , 5]);
                    return [4 /*yield*/, file_manager.save_file(paths.projects + data.currentProject + '/' + data.newFile, data.fileData, paths.lockfile)];
                case 3:
                    _a.sent();
                    if (data.checkSyntax) {
                        checkSyntax(data);
                    }
                    return [3 /*break*/, 5];
                case 4:
                    e_1 = _a.sent();
                    lock.release();
                    throw e_1;
                case 5:
                    lock.release();
                    return [2 /*return*/];
            }
        });
    });
}
exports.upload = upload;
// this function starts a syntax check
// if a syntax check or build process is in progress they are stopped
// a running program is not stopped
function checkSyntax(data) {
    if (processes.syntax.get_status()) {
        processes.syntax.stop();
        processes.syntax.queue(function () { return processes.syntax.start(data.currentProject); });
    }
    else if (processes.build.get_status()) {
        processes.build.stop();
        processes.build.queue(function () { return processes.syntax.start(data.currentProject); });
    }
    else {
        processes.syntax.start(data.currentProject);
    }
}
exports.checkSyntax = checkSyntax;
// this function is called when the run button is clicked
// if a program is already building or running it is stopped and restarted
// any syntax check in progress is stopped
function run(data) {
    if (processes.run.get_status()) {
        processes.run.stop();
        processes.run.queue(function () { return build_run(data.currentProject); });
    }
    else if (processes.build.get_status()) {
        processes.build.stop();
        processes.build.queue(function () { return build_run(data.currentProject); });
    }
    else if (processes.syntax.get_status()) {
        processes.syntax.stop();
        processes.syntax.queue(function () { return build_run(data.currentProject); });
    }
    else {
        build_run(data.currentProject);
    }
}
exports.run = run;
// this function starts a build process and when it ends it checks
// if it was stopped by a call to stop() or if there were build errors
// if neither of these are true the project is immediately run
function build_run(project) {
    processes.build.start(project);
    processes.build.queue(function (stderr, killed) {
        if (!killed && !build_error(stderr)) {
            processes.run.start(project);
        }
    });
}
// this function parses the stderr output of the build process 
// returning true if build errors (not warnings) are found
function build_error(stderr) {
    var lines = stderr.split('\n');
    for (var _i = 0, lines_1 = lines; _i < lines_1.length; _i++) {
        var line = lines_1[_i];
        var split_line = line.split(':');
        if (split_line.length >= 4) {
            if (split_line[3] === ' error' || split_line[3] === ' fatal error') {
                return true;
            }
            else if (split_line[3] === ' warning') {
                //console.log('warning');
            }
        }
    }
    return false;
}
// this function is called when the stop button is clicked
// it calls the stop() method of any running process
// if there is no running process, 'make stop' is called
function stop() {
    var stopped = false;
    if (processes.run.get_status()) {
        processes.run.stop();
        stopped = true;
    }
    if (processes.build.get_status()) {
        processes.build.stop();
        stopped = true;
    }
    if (processes.syntax.get_status()) {
        processes.syntax.stop();
        stopped = true;
    }
    if (!stopped) {
        console.log('make -C ' + paths.Bela + ' stop');
        child_process.exec('make -C ' + paths.Bela + ' stop');
    }
}
exports.stop = stop;
function get_status() {
    return {
        checkingSyntax: processes.syntax.get_status(),
        building: processes.build.get_status(),
        buildProject: (processes.build.get_status() ? processes.build.project : ''),
        running: processes.run.get_status(),
        runProject: (processes.run.get_status() ? processes.run.project : '')
    };
}
// each process emits start and finish events, which are handled here
processes.syntax.on('start', function (project) { return socket_manager.broadcast('status', get_status()); });
processes.syntax.on('finish', function (stderr) {
    var status = get_status();
    status.syntaxError = stderr;
    socket_manager.broadcast('status', status);
});
processes.build.on('start', function (project) { return socket_manager.broadcast('status', get_status()); });
processes.build.on('finish', function (stderr, killed) {
    var status = get_status();
    status.syntaxError = stderr;
    socket_manager.broadcast('status', status);
    if (!killed)
        socket_manager.broadcast('std-warn', stderr);
});
processes.build.on('stdout', function (data) { return socket_manager.broadcast('status', { buildLog: data }); });
processes.run.on('start', function (pid, project) {
    socket_manager.broadcast('status', get_status());
    cpu_monitor.start(pid, project, function (cpu) { return __awaiter(_this, void 0, void 0, function () {
        var _a, _b, _c, _d;
        return __generator(this, function (_e) {
            switch (_e.label) {
                case 0:
                    _b = (_a = socket_manager).broadcast;
                    _c = ['cpu-usage'];
                    _d = {};
                    return [4 /*yield*/, file_manager.read_file(paths.xenomai_stat).catch(function (e) { return console.log('error reading xenomai stats', e); })];
                case 1:
                    _b.apply(_a, _c.concat([(_d.bela = _e.sent(),
                            _d.belaLinux = cpu,
                            _d)]));
                    return [2 /*return*/];
            }
        });
    }); });
});
processes.run.on('finish', function (project) {
    socket_manager.broadcast('status', get_status());
    cpu_monitor.stop();
});
processes.run.on('stdout', function (data) { return socket_manager.broadcast('status', { belaLog: data }); });
processes.run.on('stderr', function (data) { return socket_manager.broadcast('status', { belaLogErr: data }); });

//# sourceMappingURL=data:application/json;charset=utf8;base64,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LFVBQUMsSUFBSSxJQUFLLE9BQUEsY0FBYyxDQUFDLFNBQVMsQ0FBQyxRQUFRLEVBQUUsRUFBQyxPQUFPLEVBQUUsSUFBSSxFQUFDLENBQUMsRUFBbkQsQ0FBbUQsQ0FBRSxDQUFDO0FBQzNGLFNBQVMsQ0FBQyxHQUFHLENBQUMsRUFBRSxDQUFDLFFBQVEsRUFBRSxVQUFDLElBQUksSUFBSyxPQUFBLGNBQWMsQ0FBQyxTQUFTLENBQUMsUUFBUSxFQUFFLEVBQUMsVUFBVSxFQUFFLElBQUksRUFBQyxDQUFDLEVBQXRELENBQXNELENBQUUsQ0FBQyIsImZpbGUiOiJQcm9jZXNzTWFuYWdlci5qcyIsInNvdXJjZXNDb250ZW50IjpbImltcG9ydCAqIGFzIGNoaWxkX3Byb2Nlc3MgZnJvbSAnY2hpbGRfcHJvY2Vzcyc7XG5pbXBvcnQgKiBhcyBmaWxlX21hbmFnZXIgZnJvbSAnLi9GaWxlTWFuYWdlcic7XG5pbXBvcnQgKiBhcyBzb2NrZXRfbWFuYWdlciBmcm9tICcuL1NvY2tldE1hbmFnZXInO1xuaW1wb3J0ICogYXMgcHJvY2Vzc2VzIGZyb20gJy4vSURFUHJvY2Vzc2VzJztcbmltcG9ydCAqIGFzIHBhdGhzIGZyb20gJy4vcGF0aHMnO1xuaW1wb3J0ICogYXMgdXRpbCBmcm9tICcuL3V0aWxzJztcbmltcG9ydCB7IExvY2sgfSBmcm9tICcuL0xvY2snO1xuaW1wb3J0ICogYXMgY3B1X21vbml0b3IgZnJvbSAnLi9DUFVNb25pdG9yJztcblxuY29uc3QgbG9jazogTG9jayA9IG5ldyBMb2NrKCk7XG5cbi8vIHRoaXMgZnVuY3Rpb24gZ2V0cyBjYWxsZWQgd2hlbmV2ZXIgdGhlIGFjZSBlZGl0b3IgaXMgbW9kaWZpZWRcbi8vIHRoZSBmaWxlIGRhdGEgaXMgc2F2ZWQgcm9idXN0bHkgdXNpbmcgYSBsb2NrZmlsZSwgYW5kIGEgc3ludGF4XG4vLyBjaGVjayBzdGFydGVkIGlmIHRoZSBmbGFnIGlzIHNldFxuZXhwb3J0IGFzeW5jIGZ1bmN0aW9uIHVwbG9hZChkYXRhOiBhbnkpe1xuXHRhd2FpdCBsb2NrLmFjcXVpcmUoKTtcblx0dHJ5e1xuXHRcdGF3YWl0IGZpbGVfbWFuYWdlci5zYXZlX2ZpbGUocGF0aHMucHJvamVjdHMrZGF0YS5jdXJyZW50UHJvamVjdCsnLycrZGF0YS5uZXdGaWxlLCBkYXRhLmZpbGVEYXRhLCBwYXRocy5sb2NrZmlsZSk7XG5cdFx0aWYgKGRhdGEuY2hlY2tTeW50YXgpe1xuXHRcdFx0Y2hlY2tTeW50YXgoZGF0YSk7XG5cdFx0fVxuXHR9XG5cdGNhdGNoKGUpe1xuXHRcdGxvY2sucmVsZWFzZSgpO1xuXHRcdHRocm93IGU7XG5cdH1cblx0bG9jay5yZWxlYXNlKCk7XG59XG5cbi8vIHRoaXMgZnVuY3Rpb24gc3RhcnRzIGEgc3ludGF4IGNoZWNrXG4vLyBpZiBhIHN5bnRheCBjaGVjayBvciBidWlsZCBwcm9jZXNzIGlzIGluIHByb2dyZXNzIHRoZXkgYXJlIHN0b3BwZWRcbi8vIGEgcnVubmluZyBwcm9ncmFtIGlzIG5vdCBzdG9wcGVkXG5leHBvcnQgZnVuY3Rpb24gY2hlY2tTeW50YXgoZGF0YTogYW55KXtcblx0aWYgKHByb2Nlc3Nlcy5zeW50YXguZ2V0X3N0YXR1cygpKXtcblx0XHRwcm9jZXNzZXMuc3ludGF4LnN0b3AoKTtcblx0XHRwcm9jZXNzZXMuc3ludGF4LnF1ZXVlKCgpID0+IHByb2Nlc3Nlcy5zeW50YXguc3RhcnQoZGF0YS5jdXJyZW50UHJvamVjdCkpO1xuXHR9IGVsc2UgaWYgKHByb2Nlc3Nlcy5idWlsZC5nZXRfc3RhdHVzKCkpe1xuXHRcdHByb2Nlc3Nlcy5idWlsZC5zdG9wKCk7XG5cdFx0cHJvY2Vzc2VzLmJ1aWxkLnF1ZXVlKCAoKSA9PiBwcm9jZXNzZXMuc3ludGF4LnN0YXJ0KGRhdGEuY3VycmVudFByb2plY3QpICk7XG5cdH0gZWxzZSB7XG5cdFx0cHJvY2Vzc2VzLnN5bnRheC5zdGFydChkYXRhLmN1cnJlbnRQcm9qZWN0KTtcblx0fVxufVxuXG4vLyB0aGlzIGZ1bmN0aW9uIGlzIGNhbGxlZCB3aGVuIHRoZSBydW4gYnV0dG9uIGlzIGNsaWNrZWRcbi8vIGlmIGEgcHJvZ3JhbSBpcyBhbHJlYWR5IGJ1aWxkaW5nIG9yIHJ1bm5pbmcgaXQgaXMgc3RvcHBlZCBhbmQgcmVzdGFydGVkXG4vLyBhbnkgc3ludGF4IGNoZWNrIGluIHByb2dyZXNzIGlzIHN0b3BwZWRcbmV4cG9ydCBmdW5jdGlvbiBydW4oZGF0YTogYW55KXtcblx0aWYgKHByb2Nlc3Nlcy5ydW4uZ2V0X3N0YXR1cygpKXtcblx0XHRwcm9jZXNzZXMucnVuLnN0b3AoKTtcblx0XHRwcm9jZXNzZXMucnVuLnF1ZXVlKCAoKSA9PiBidWlsZF9ydW4oZGF0YS5jdXJyZW50UHJvamVjdCkgKTtcblx0fSBlbHNlIGlmIChwcm9jZXNzZXMuYnVpbGQuZ2V0X3N0YXR1cygpKXtcblx0XHRwcm9jZXNzZXMuYnVpbGQuc3RvcCgpO1xuXHRcdHByb2Nlc3Nlcy5idWlsZC5xdWV1ZSggKCkgPT4gYnVpbGRfcnVuKGRhdGEuY3VycmVudFByb2plY3QpICk7XG5cdH0gZWxzZSBpZiAocHJvY2Vzc2VzLnN5bnRheC5nZXRfc3RhdHVzKCkpe1xuXHRcdHByb2Nlc3Nlcy5zeW50YXguc3RvcCgpO1xuXHRcdHByb2Nlc3Nlcy5zeW50YXgucXVldWUoICgpID0+IGJ1aWxkX3J1bihkYXRhLmN1cnJlbnRQcm9qZWN0KSApO1x0XG5cdH0gZWxzZSB7XG5cdFx0YnVpbGRfcnVuKGRhdGEuY3VycmVudFByb2plY3QpO1xuXHR9XG59XG5cbi8vIHRoaXMgZnVuY3Rpb24gc3RhcnRzIGEgYnVpbGQgcHJvY2VzcyBhbmQgd2hlbiBpdCBlbmRzIGl0IGNoZWNrc1xuLy8gaWYgaXQgd2FzIHN0b3BwZWQgYnkgYSBjYWxsIHRvIHN0b3AoKSBvciBpZiB0aGVyZSB3ZXJlIGJ1aWxkIGVycm9yc1xuLy8gaWYgbmVpdGhlciBvZiB0aGVzZSBhcmUgdHJ1ZSB0aGUgcHJvamVjdCBpcyBpbW1lZGlhdGVseSBydW5cbmZ1bmN0aW9uIGJ1aWxkX3J1bihwcm9qZWN0OiBzdHJpbmcpe1xuXHRwcm9jZXNzZXMuYnVpbGQuc3RhcnQocHJvamVjdCk7XG5cdHByb2Nlc3Nlcy5idWlsZC5xdWV1ZSggKHN0ZGVycjogc3RyaW5nLCBraWxsZWQ6IGJvb2xlYW4pID0+IHtcblx0XHRpZiAoIWtpbGxlZCAmJiAhYnVpbGRfZXJyb3Ioc3RkZXJyKSl7XG5cdFx0XHRwcm9jZXNzZXMucnVuLnN0YXJ0KHByb2plY3QpOyBcblx0XHR9XG5cdH0pO1xufVxuXG4vLyB0aGlzIGZ1bmN0aW9uIHBhcnNlcyB0aGUgc3RkZXJyIG91dHB1dCBvZiB0aGUgYnVpbGQgcHJvY2VzcyBcbi8vIHJldHVybmluZyB0cnVlIGlmIGJ1aWxkIGVycm9ycyAobm90IHdhcm5pbmdzKSBhcmUgZm91bmRcbmZ1bmN0aW9uIGJ1aWxkX2Vycm9yKHN0ZGVycjogc3RyaW5nKTogYm9vbGVhbiB7XG5cdGxldCBsaW5lczogc3RyaW5nW10gPSBzdGRlcnIuc3BsaXQoJ1xcbicpO1xuXHRmb3IgKGxldCBsaW5lIG9mIGxpbmVzKXtcblx0XHRsZXQgc3BsaXRfbGluZTogc3RyaW5nW10gPSBsaW5lLnNwbGl0KCc6Jyk7XG5cdFx0aWYgKHNwbGl0X2xpbmUubGVuZ3RoID49IDQpe1xuXHRcdFx0aWYgKHNwbGl0X2xpbmVbM10gPT09ICcgZXJyb3InIHx8IHNwbGl0X2xpbmVbM10gPT09ICcgZmF0YWwgZXJyb3InKXtcblx0XHRcdFx0cmV0dXJuIHRydWU7XG5cdFx0XHR9IGVsc2UgaWYgKHNwbGl0X2xpbmVbM10gPT09ICcgd2FybmluZycpe1xuXHRcdFx0XHQvL2NvbnNvbGUubG9nKCd3YXJuaW5nJyk7XG5cdFx0XHR9XG5cdFx0fVxuXHR9XG5cdHJldHVybiBmYWxzZTtcbn1cblxuXG4vLyB0aGlzIGZ1bmN0aW9uIGlzIGNhbGxlZCB3aGVuIHRoZSBzdG9wIGJ1dHRvbiBpcyBjbGlja2VkXG4vLyBpdCBjYWxscyB0aGUgc3RvcCgpIG1ldGhvZCBvZiBhbnkgcnVubmluZyBwcm9jZXNzXG4vLyBpZiB0aGVyZSBpcyBubyBydW5uaW5nIHByb2Nlc3MsICdtYWtlIHN0b3AnIGlzIGNhbGxlZFxuZXhwb3J0IGZ1bmN0aW9uIHN0b3AoKXtcblx0bGV0IHN0b3BwZWQ6IGJvb2xlYW4gPSBmYWxzZTtcblx0aWYgKHByb2Nlc3Nlcy5ydW4uZ2V0X3N0YXR1cygpKXtcblx0XHRwcm9jZXNzZXMucnVuLnN0b3AoKTtcblx0XHRzdG9wcGVkID0gdHJ1ZTtcblx0fVxuXHRpZiAocHJvY2Vzc2VzLmJ1aWxkLmdldF9zdGF0dXMoKSl7XG5cdFx0cHJvY2Vzc2VzLmJ1aWxkLnN0b3AoKTtcblx0XHRzdG9wcGVkID0gdHJ1ZTtcblx0fVxuXHRpZiAocHJvY2Vzc2VzLnN5bnRheC5nZXRfc3RhdHVzKCkpe1xuXHRcdHByb2Nlc3Nlcy5zeW50YXguc3RvcCgpO1xuXHRcdHN0b3BwZWQgPSB0cnVlO1xuXHR9XG5cdGlmICghc3RvcHBlZCl7XG5cdFx0Y29uc29sZS5sb2coJ21ha2UgLUMgJytwYXRocy5CZWxhKycgc3RvcCcpO1xuXHRcdGNoaWxkX3Byb2Nlc3MuZXhlYygnbWFrZSAtQyAnK3BhdGhzLkJlbGErJyBzdG9wJyk7XHRcblx0fVxufVxuXG5mdW5jdGlvbiBnZXRfc3RhdHVzKCk6IHV0aWwuUHJvY2Vzc19TdGF0dXMge1xuXHRyZXR1cm4ge1xuXHRcdGNoZWNraW5nU3ludGF4XHQ6IHByb2Nlc3Nlcy5zeW50YXguZ2V0X3N0YXR1cygpLFxuXHRcdGJ1aWxkaW5nXHQ6IHByb2Nlc3Nlcy5idWlsZC5nZXRfc3RhdHVzKCksXG5cdFx0YnVpbGRQcm9qZWN0XHQ6IChwcm9jZXNzZXMuYnVpbGQuZ2V0X3N0YXR1cygpID8gcHJvY2Vzc2VzLmJ1aWxkLnByb2plY3QgOiAnJyksXG5cdFx0cnVubmluZ1x0XHQ6IHByb2Nlc3Nlcy5ydW4uZ2V0X3N0YXR1cygpLFxuXHRcdHJ1blByb2plY3RcdDogKHByb2Nlc3Nlcy5ydW4uZ2V0X3N0YXR1cygpID8gcHJvY2Vzc2VzLnJ1bi5wcm9qZWN0IDogJycpXG5cdH07XG59XG5cbi8vIGVhY2ggcHJvY2VzcyBlbWl0cyBzdGFydCBhbmQgZmluaXNoIGV2ZW50cywgd2hpY2ggYXJlIGhhbmRsZWQgaGVyZVxucHJvY2Vzc2VzLnN5bnRheC5vbignc3RhcnQnLCAocHJvamVjdDogc3RyaW5nKSA9PiBzb2NrZXRfbWFuYWdlci5icm9hZGNhc3QoJ3N0YXR1cycsIGdldF9zdGF0dXMoKSkgKTtcbnByb2Nlc3Nlcy5zeW50YXgub24oJ2ZpbmlzaCcsIChzdGRlcnI6IHN0cmluZykgPT4ge1xuXHRsZXQgc3RhdHVzOiB1dGlsLlByb2Nlc3NfU3RhdHVzID0gZ2V0X3N0YXR1cygpO1xuXHRzdGF0dXMuc3ludGF4RXJyb3IgPSBzdGRlcnI7XG5cdHNvY2tldF9tYW5hZ2VyLmJyb2FkY2FzdCgnc3RhdHVzJywgc3RhdHVzKTtcbn0pO1xuXG5wcm9jZXNzZXMuYnVpbGQub24oJ3N0YXJ0JywgKHByb2plY3Q6IHN0cmluZykgPT4gc29ja2V0X21hbmFnZXIuYnJvYWRjYXN0KCdzdGF0dXMnLCBnZXRfc3RhdHVzKCkpICk7XG5wcm9jZXNzZXMuYnVpbGQub24oJ2ZpbmlzaCcsIChzdGRlcnI6IHN0cmluZywga2lsbGVkOiBib29sZWFuKSA9PiB7XG5cdGxldCBzdGF0dXM6IHV0aWwuUHJvY2Vzc19TdGF0dXMgPSBnZXRfc3RhdHVzKCk7XG5cdHN0YXR1cy5zeW50YXhFcnJvciA9IHN0ZGVycjtcblx0c29ja2V0X21hbmFnZXIuYnJvYWRjYXN0KCdzdGF0dXMnLCBzdGF0dXMpO1xuXHRpZiAoIWtpbGxlZClcblx0XHRzb2NrZXRfbWFuYWdlci5icm9hZGNhc3QoJ3N0ZC13YXJuJywgc3RkZXJyKTtcbn0pO1xucHJvY2Vzc2VzLmJ1aWxkLm9uKCdzdGRvdXQnLCAoZGF0YSkgPT4gc29ja2V0X21hbmFnZXIuYnJvYWRjYXN0KCdzdGF0dXMnLCB7YnVpbGRMb2c6IGRhdGF9KSApO1xuXG5wcm9jZXNzZXMucnVuLm9uKCdzdGFydCcsIChwaWQ6IG51bWJlciwgcHJvamVjdDogc3RyaW5nKSA9PiB7XG5cdHNvY2tldF9tYW5hZ2VyLmJyb2FkY2FzdCgnc3RhdHVzJywgZ2V0X3N0YXR1cygpKTtcblx0Y3B1X21vbml0b3Iuc3RhcnQocGlkLCBwcm9qZWN0LCBhc3luYyBjcHUgPT4ge1xuXHRcdHNvY2tldF9tYW5hZ2VyLmJyb2FkY2FzdCgnY3B1LXVzYWdlJywge1xuXHRcdFx0YmVsYTogYXdhaXQgZmlsZV9tYW5hZ2VyLnJlYWRfZmlsZShwYXRocy54ZW5vbWFpX3N0YXQpLmNhdGNoKGUgPT4gY29uc29sZS5sb2coJ2Vycm9yIHJlYWRpbmcgeGVub21haSBzdGF0cycsIGUpKSxcblx0XHRcdGJlbGFMaW51eDogY3B1XG5cdFx0fSk7XG5cdH0pO1xufSk7XG5wcm9jZXNzZXMucnVuLm9uKCdmaW5pc2gnLCAocHJvamVjdDogc3RyaW5nKSA9PiB7XG5cdHNvY2tldF9tYW5hZ2VyLmJyb2FkY2FzdCgnc3RhdHVzJywgZ2V0X3N0YXR1cygpKTtcblx0Y3B1X21vbml0b3Iuc3RvcCgpO1xufSk7XG5wcm9jZXNzZXMucnVuLm9uKCdzdGRvdXQnLCAoZGF0YSkgPT4gc29ja2V0X21hbmFnZXIuYnJvYWRjYXN0KCdzdGF0dXMnLCB7YmVsYUxvZzogZGF0YX0pICk7XG5wcm9jZXNzZXMucnVuLm9uKCdzdGRlcnInLCAoZGF0YSkgPT4gc29ja2V0X21hbmFnZXIuYnJvYWRjYXN0KCdzdGF0dXMnLCB7YmVsYUxvZ0VycjogZGF0YX0pICk7XG4iXX0=
