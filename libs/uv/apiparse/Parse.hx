import sys.io.File;

using StringTools;

class Parse {
	public static function main():Void {
		function getFunctions(path:String):Array<String> {
			return File.getContent(path).split("\n").filter(line -> line.startsWith(".. c:function:: "));
		}
		var fsRE = ~/\.\. c:function:: int uv_fs_([^\(]+)\(uv_loop_t\* loop, uv_fs_t\* req, (([a-z\*\[\]_ 0-9]+, )+)uv_fs_cb cb\)/;
		var wrap = [];
		var haxe = [];
		for (f in getFunctions("fs.rst")) {
			if (!fsRE.match(f)) continue;
			var name = fsRE.matched(1);
			// skip functions that are not in 1.8.0
			// could parse ".. versionadded" but this is simpler
			if ([
					"opendir",
					"closedir",
					"readdir",
					"copyfile",
					"lchown"
				].indexOf(name) != -1) continue;
			var signature = fsRE.matched(2);
			signature = signature.substr(0, signature.length - 2); // remove last comma
			var ffi = [];
			var haxeArgs = [];
			var wrapArgs = [ for (arg in signature.split(", ")) {
					var argSplit = arg.split(" ");
					var argName = argSplit.pop();
					var argType = argSplit.join(" ");
					var isArr = argName.endsWith("[]");
					if (isArr) argType += "*";
					var modArgs = (switch (argType) {
							case "uv_file": ["_FILE", "file:UVFile"];
							case "uv_dir_t*": ["_DIR", "dir:UVDir"];
							case "const uv_buf_t*" if (isArr): ["_ARR", "_:hl.NativeArray<UVBuf>"];
							case "uv_uid_t" | "uv_gid_t": ["_I32", "_:Int"]; // might be system specific?
							
							case "int" | "unsigned int": ["_I32", "_:Int"];
							case "int64_t" | "size_t": ["_I64", "_:hl.I64"];
							case "double": ["_F64", "_:Float"];
							case "const char*": ["_BYTES", "_:hl.Bytes"];
							
							case _: throw argType;
						});
					ffi.push(modArgs[0]);
					haxeArgs.push(modArgs.length > 1 ? modArgs[1] : "_:Dynamic");
					argType;
				} ];
			wrap.push('UV_WRAP${wrapArgs.length}(fs_$name, ${wrapArgs.join(", ")}, ${ffi.join(" ")});');
			haxe.push('@:hlNative("uv", "w_fs_$name") public static function fs_$name(loop:UVLoop, ${haxeArgs.join(", ")}, cb:UVError->Void):Void {}');
		}
		Sys.println(wrap.join("\n"));
		Sys.println(haxe.join("\n"));
	}
}
