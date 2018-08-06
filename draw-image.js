/*
 * Copyright 2018 Google LLC. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
function drawImageToCanvas(pixels, width, height) {

  var canvas = document.getElementById('view');
  canvas.width = width;
  canvas.height = height;

  var gl = canvas.getContext('webgl', { preserveDrawingBuffer : true });
  if (!gl)
    throw new Error('FAIL: could not create webgl canvas context');

  var colorCorrect = gl.BROWSER_DEFAULT_WEBGL;
  gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, colorCorrect);
  gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

  gl.viewport(0, 0, canvas.width, canvas.height);
  gl.clearColor(0, 0, 0, 1);
  gl.clear(gl.COLOR_BUFFER_BIT);

  if (gl.getError() != gl.NONE)
    throw new Error('FAIL: webgl canvas context setup failed');

  function createShader(id, type) {
    var shader = gl.createShader(type);
    gl.shaderSource(shader, document.getElementById(id).text);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS))
      throw new Error('FAIL: shader ' + id + ' compilation failed');
    return shader;
  }

  var program = gl.createProgram();
  gl.attachShader(program, createShader('vs', gl.VERTEX_SHADER));
  gl.attachShader(program, createShader('fs', gl.FRAGMENT_SHADER));
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS))
    throw new Error('FAIL: webgl shader program linking failed');
  gl.useProgram(program);

  var texture = gl.createTexture();
  gl.activeTexture(gl.TEXTURE0);
  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, width, height, 0, gl.RGB, gl.UNSIGNED_BYTE, pixels);

  if (gl.getError() != gl.NONE)
    throw new Error('FAIL: creating webgl image texture failed');

  function createBuffer(data) {
    var buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);
    return buffer;
  }

  var vertexCoords = new Float32Array([-1, 1, -1, -1, 1, -1, 1, 1]);
  var vertexBuffer = createBuffer(vertexCoords);
  var location = gl.getAttribLocation(program, 'aVertex');
  gl.enableVertexAttribArray(location);
  gl.vertexAttribPointer(location, 2, gl.FLOAT, false, 0, 0);

  if (gl.getError() != gl.NONE)
    throw new Error('FAIL: vertex-coord setup failed');

  var texCoords = new Float32Array([0, 1, 0, 0, 1, 0, 1, 1]);
  var texBuffer = createBuffer(texCoords);
  var location = gl.getAttribLocation(program, 'aTex');
  gl.enableVertexAttribArray(location);
  gl.vertexAttribPointer(location, 2, gl.FLOAT, false, 0, 0);

  if (gl.getError() != gl.NONE)
    throw new Error('FAIL: tex-coord setup setup failed');

  gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);  // Render the image.
}
