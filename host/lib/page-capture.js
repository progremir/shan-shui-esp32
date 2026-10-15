// Runs inside the generator page. Installed by toString(); keep it free of
// Node closures. It drives the unmodified shan-shui-inf update() and rasters
// one device-pixel window of the live SVG, paper texture included.
export function ssCapture(deviceW, deviceH, userX, userW, userH) {
  return new Promise(function (resolve, reject) {
    try {
      if (typeof MEM !== "undefined" && MEM.chunks && MEM.chunks.length > 40) {
        var horizon = userX - 4000;
        var dst = 0;
        for (var i = 0; i < MEM.chunks.length; i++) {
          if (MEM.chunks[i].x >= horizon) MEM.chunks[dst++] = MEM.chunks[i];
        }
        MEM.chunks.length = dst;
      }
      var left = Math.max(0, userX - 240);
      MEM.cursx = left;
      MEM.windx = Math.max(64, userX + userW - left + 240);
      if (userH > MEM.windy) MEM.windy = userH;
      update();
      var svg = document.getElementById("SVG");
      if (!svg) {
        reject("shan-shui-inf did not produce an SVG");
        return;
      }
      svg.setAttribute("width", String(deviceW));
      svg.setAttribute("height", String(deviceH));
      svg.setAttribute("viewBox", userX + " 0 " + userW + " " + userH);
      var NS = "http://www.w3.org/2000/svg";
      var g = document.getElementById("G");
      g.setAttribute("style", "mix-blend-mode:multiply");
      var defs = document.getElementById("PAPER_DEFS");
      if (!defs) {
        defs = document.createElementNS(NS, "defs");
        defs.setAttribute("id", "PAPER_DEFS");
        svg.insertBefore(defs, svg.firstChild);
      }
      while (defs.firstChild) defs.removeChild(defs.firstChild);
      var paper = document.getElementById("bgcanv");
      var href = paper.toDataURL("image/png");
      var cell = 512 * (userW / deviceW);
      var pattern = document.createElementNS(NS, "pattern");
      pattern.setAttribute("id", "paperpat");
      pattern.setAttribute("patternUnits", "userSpaceOnUse");
      pattern.setAttribute("x", String(userX));
      pattern.setAttribute("y", "0");
      pattern.setAttribute("width", String(cell));
      pattern.setAttribute("height", String(cell));
      var pim = document.createElementNS(NS, "image");
      pim.setAttribute("href", href);
      pim.setAttributeNS("http://www.w3.org/1999/xlink", "href", href);
      pim.setAttribute("width", String(cell));
      pim.setAttribute("height", String(cell));
      pim.setAttribute("preserveAspectRatio", "none");
      pattern.appendChild(pim);
      defs.appendChild(pattern);
      var rect = document.getElementById("PAPER_RECT");
      if (!rect) {
        rect = document.createElementNS(NS, "rect");
        rect.setAttribute("id", "PAPER_RECT");
        svg.insertBefore(rect, g);
      }
      rect.setAttribute("x", String(userX));
      rect.setAttribute("y", "0");
      rect.setAttribute("width", String(userW));
      rect.setAttribute("height", String(userH));
      rect.setAttribute("fill", "url(#paperpat)");
      var xml = new XMLSerializer().serializeToString(svg);
      var blob = new Blob([xml], { type: "image/svg+xml;charset=utf-8" });
      var url = URL.createObjectURL(blob);
      var image = new Image();
      image.onload = function () {
        try {
          var canvas = document.createElement("canvas");
          canvas.width = deviceW;
          canvas.height = deviceH;
          var ctx = canvas.getContext("2d", { willReadFrequently: true });
          ctx.drawImage(image, 0, 0, deviceW, deviceH);
          var data = ctx.getImageData(0, 0, deviceW, deviceH).data;
          var gray = new Uint8Array(deviceW * deviceH);
          for (var i = 0, p = 0; i < data.length; i += 4, p++) {
            gray[p] = (data[i] * 299 + data[i + 1] * 587 + data[i + 2] * 114 + 500) / 1000;
          }
          var s = "";
          var step = 8192;
          for (var o = 0; o < gray.length; o += step) {
            s += String.fromCharCode.apply(null, gray.subarray(o, Math.min(gray.length, o + step)));
          }
          URL.revokeObjectURL(url);
          resolve(btoa(s));
        } catch (err) {
          reject(String(err && err.stack ? err.stack : err));
        }
      };
      image.onerror = function () {
        URL.revokeObjectURL(url);
        reject("SVG raster failed");
      };
      image.src = url;
    } catch (err) {
      reject(String(err && err.stack ? err.stack : err));
    }
  });
}
