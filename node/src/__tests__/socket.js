import net from 'net'

function main() {
  const host = '0.0.0.0'
  const port = 20010

  const s2 = net.createServer((c) => {
    console.log('c created')
    c.setTimeout(3000)
    c.on('data', (data) => {
      console.log('c data: ', data)
    })
    c.on('error', (err) => {
      console.log('c error: ', err)
    })
    c.on('close', () => {
      console.log('c closed')
    })
  })

  s2.listen({
    port,
    host,
  })

  const s1 = net.createConnection({
    host,
    port,
  })

  s1.on('data', (data) => {
    console.log('s1 data: ', data)
  })
  s1.on('error', (err) => {
    console.log('s1 error: ', err)
  })
  s1.on('close', () => {
    console.log('s1 closed')
  })

  s1.write(Buffer.from('Hello'))
  // s1.end()
}

if (module === require.main) {
  main()
}
