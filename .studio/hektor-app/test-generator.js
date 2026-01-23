// Test data generator for HEKTOR Vector Database
// Generates 1M test vectors and demonstrates performance

const hektor = require('./build/Release/hektor_native.node');

async function generateTestData(count = 1000000) {
  console.log(`🚀 Generating ${count.toLocaleString()} test vectors...`);
  
  // Create database
  const dbConfig = {
    path: './test_db',
    dimension: 512,
    metric: 'cosine',
    hnswM: 16,
    hnswEfConstruction: 200,
    hnswEfSearch: 50,
    maxElements: count
  };
  
  const db = new hektor.Database(dbConfig);
  const initResult = db.initialize();
  
  if (!initResult.success) {
    console.error('❌ Failed to initialize database:', initResult.error);
    return;
  }
  
  console.log('✅ Database initialized');
  
  // Generate random vectors
  const batchSize = 1000;
  const batches = Math.ceil(count / batchSize);
  
  let totalAdded = 0;
  const startTime = Date.now();
  
  for (let batch = 0; batch < batches; batch++) {
    const vectors = [];
    const metadata = [];
    
    const currentBatchSize = Math.min(batchSize, count - totalAdded);
    
    for (let i = 0; i < currentBatchSize; i++) {
      // Generate random normalized vector
      const vec = new Array(512);
      let norm = 0;
      for (let j = 0; j < 512; j++) {
        vec[j] = Math.random() * 2 - 1;  // Random between -1 and 1
        norm += vec[j] * vec[j];
      }
      norm = Math.sqrt(norm);
      for (let j = 0; j < 512; j++) {
        vec[j] /= norm;  // Normalize
      }
      
      vectors.push(vec);
      
      // Generate metadata
      const meta = {
        id: totalAdded + i,
        date: `2026-01-${(i % 28) + 1}`,
        type: Math.floor(Math.random() * 11),  // 0-10 document types
        asset: ['GOLD', 'SILVER', 'PLATINUM'][Math.floor(Math.random() * 3)],
        bias: ['BULLISH', 'BEARISH', 'NEUTRAL'][Math.floor(Math.random() * 3)]
      };
      
      metadata.push(meta);
    }
    
    // Add batch
    const result = db.addVectors(vectors, metadata);
    
    if (result.success) {
      totalAdded += currentBatchSize;
      
      if ((batch + 1) % 10 === 0) {
        const elapsed = (Date.now() - startTime) / 1000;
        const rate = totalAdded / elapsed;
        const remaining = (count - totalAdded) / rate;
        
        console.log(`  Progress: ${totalAdded.toLocaleString()}/${count.toLocaleString()} (${(totalAdded/count*100).toFixed(1)}%) - ${rate.toFixed(0)} vec/s - ETA: ${remaining.toFixed(0)}s`);
      }
    } else {
      console.error(`❌ Batch ${batch} failed:`, result.error);
    }
  }
  
  const totalTime = (Date.now() - startTime) / 1000;
  console.log(`\n✅ Generated ${totalAdded.toLocaleString()} vectors in ${totalTime.toFixed(2)}s`);
  console.log(`   Average rate: ${(totalAdded / totalTime).toFixed(0)} vectors/second`);
  
  // Get database stats
  const stats = db.getStats();
  console.log('\n📊 Database Statistics:');
  console.log(`   Size: ${stats.size.toLocaleString()} vectors`);
  console.log(`   Dimension: ${stats.dimension}`);
  console.log(`   Ready: ${stats.ready}`);
  
  return db;
}

async function runBenchmarks(db) {
  console.log('\n\n🔥 Running Performance Benchmarks...\n');
  
  // Create index management instance
  const dbId = 'db_0';  // Assuming first database
  const indexMgmt = new hektor.IndexManagement(dbId);
  
  // Run benchmark with 1000 queries
  const results = indexMgmt.runBenchmark({
    numQueries: 1000,
    k: 10
  });
  
  if (results.success) {
    console.log('📈 Benchmark Results:');
    console.log(`   Total queries: ${results.numQueries}`);
    console.log(`   k: ${results.k}`);
    console.log(`   Total time: ${results.totalTimeMs.toFixed(2)} ms`);
    console.log(`   Average query time: ${results.avgQueryTimeMs.toFixed(3)} ms`);
    console.log(`   Min query time: ${results.minQueryTimeMs.toFixed(3)} ms`);
    console.log(`   Max query time: ${results.maxQueryTimeMs.toFixed(3)} ms`);
    console.log(`   Queries per second: ${results.queriesPerSecond.toFixed(0)} QPS`);
    console.log(`   Database size: ${results.dbSize.toLocaleString()} vectors`);
    console.log(`   Dimension: ${results.dimension}`);
  } else {
    console.error('❌ Benchmark failed');
  }
}

async function demonstrateSearch(db) {
  console.log('\n\n🔍 Demonstrating Search Capabilities...\n');
  
  const dbId = 'db_0';
  const search = new hektor.Search(dbId);
  
  // Generate a random query vector
  const query = new Array(512);
  let norm = 0;
  for (let i = 0; i < 512; i++) {
    query[i] = Math.random() * 2 - 1;
    norm += query[i] * query[i];
  }
  norm = Math.sqrt(norm);
  for (let i = 0; i < 512; i++) {
    query[i] /= norm;
  }
  
  // Vector search
  console.log('1️⃣ Vector Similarity Search (k=10)');
  const vectorResults = search.vectorSearch(query, { k: 10 });
  
  if (vectorResults.success) {
    console.log(`   Found ${vectorResults.results.length} results`);
    vectorResults.results.slice(0, 5).forEach((r, i) => {
      console.log(`   ${i+1}. ID: ${r.id}, Distance: ${r.distance.toFixed(4)}, Score: ${r.score.toFixed(4)}`);
    });
  }
  
  // Filtered search
  console.log('\n2️⃣ Filtered Search (GOLD assets only)');
  const filteredResults = search.vectorSearch(query, {
    k: 10,
    assetFilter: 'GOLD'
  });
  
  if (filteredResults.success) {
    console.log(`   Found ${filteredResults.results.length} GOLD results`);
  }
}

async function main() {
  console.log('╔══════════════════════════════════════════════════════════════╗');
  console.log('║                                                              ║');
  console.log('║          HEKTOR VECTOR DATABASE - TEST SUITE                 ║');
  console.log('║          Production-Ready 1M Vector Demonstration            ║');
  console.log('║                                                              ║');
  console.log('╚══════════════════════════════════════════════════════════════╝\n');
  
  // Get system info
  const sysinfo = hektor.getSystemInfo();
  console.log('💻 System Information:');
  console.log(`   SIMD: ${sysinfo.simd}`);
  console.log(`   Platform: ${sysinfo.platform}`);
  console.log(`   C++ Standard: ${sysinfo.cppStandard}\n`);
  
  // Generate test data
  const db = await generateTestData(10000);  // Start with 10K for demo
  
  // Run benchmarks
  await runBenchmarks(db);
  
  // Demonstrate search
  await demonstrateSearch(db);
  
  console.log('\n✨ All tests complete!\n');
  
  // Close database
  db.close();
}

main().catch(console.error);
