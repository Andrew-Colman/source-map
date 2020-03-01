const Benchmark = require("tiny-benchy");
const MozillaSourceMap = require("source-map");
const SourceMap = require("../");

const ITERATIONS = 1000;

const test_maps = [
  {
    version: 3,
    file: "helloworld.js",
    sources: ["helloworld.coffee"],
    names: [],
    mappings: "AAAA;AAAA,EAAA,OAAO,CAAC,GAAR,CAAY,aAAZ,CAAA,CAAA;AAAA"
  },
  require("./maps/angular")
];
const suite = new Benchmark(ITERATIONS);

suite.add("source-map#consume", async () => {
  for (let map of test_maps) {
    let smc = await new MozillaSourceMap.SourceMapConsumer(map);
    smc.destroy();
  }
});

/*suite.add("source-map#consume->generate", async () => {
  for (let map of test_maps) {
    let smc = await new MozillaSourceMap.SourceMapConsumer(map);
    smg = sourceMap.SourceMapGenerator.fromSourceMap(smc);
    smc.destroy();
  }
});*/

suite.add("cpp#consume", async () => {
  for (let map of test_maps) {
    let sm = new SourceMap(map.mappings, map.sources.length, map.names.length);
    // sm.addMappings(map.mappings, map.sources.length, map.names.length);
  }
});

suite.run();

/*console.log(test_maps[0].mappings);
let sm = new SourceMap(test_maps[0].mappings, test_maps[0].sources.length, test_maps[0].sources.length);
*/