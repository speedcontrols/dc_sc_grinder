// Second order butterworth low pass filter

// Calculate filter parameters
exports.create = function (sample_rate, cutoff) {
  const w = Math.tan(Math.PI * cutoff / sample_rate)
  const norm = 1.0 / (w * (w + Math.SQRT2) + 1.0)

  const a1 = 2.0 * norm * (w * w - 1.0)
  const a2 = norm * (w * (w - Math.SQRT2) + 1.0)

  const b1 = 2.0 * norm *w * w
  const b0 = 0.5 * b1
  const b2 = b0

  return Float32Array.from([a1, a2, b0, b1, b2])
}

// Apply filter to array of data
exports.filter = function (input, params) {
  const [a1, a2, b0, b1, b2] = params
  const u = Float32Array.from([input[0], input[1], input[2]])
  const output = new Float32Array(input.length)

  for (let i = 0; i < input.length; i++) {
    u[0] = u[1]
    u[1] = u[2]

    u[2] = input[i] - a1 * u[1] - a2 * u[0]

    output[i] = b0 * u[2] + b1 * u[1] + b2 * u[0]
  }

  return output
}
