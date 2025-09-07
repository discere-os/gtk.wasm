#!/usr/bin/env node
/**
 * Browser-based GTK WASM Test Runner using Puppeteer
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

const puppeteer = require('puppeteer');
const fs = require('fs');
const path = require('path');

class BrowserTestRunner {
    constructor() {
        this.browser = null;
        this.page = null;
        this.testResults = {};
    }

    async run() {
        try {
            console.log('🌐 Starting Browser Test Runner');
            
            await this.setupBrowser();
            await this.runTests();
            await this.generateReport();
            
        } catch (error) {
            console.error('❌ Browser test failed:', error.message);
            process.exit(1);
        } finally {
            await this.cleanup();
        }
    }

    async setupBrowser() {
        console.log('🚀 Launching browser...');
        
        this.browser = await puppeteer.launch({
            headless: 'new',
            args: [
                '--no-sandbox',
                '--disable-setuid-sandbox',
                '--enable-features=WebAssemblyThreads,WebAssemblySIMD',
                '--disable-web-security',
                '--allow-running-insecure-content',
                '--disable-features=VizDisplayCompositor'
            ]
        });

        this.page = await this.browser.newPage();
        
        // Enable console logging
        this.page.on('console', msg => {
            console.log(`Browser: ${msg.text()}`);
        });

        this.page.on('pageerror', error => {
            console.error(`Page error: ${error.message}`);
        });

        // Set up page with WASM support
        await this.page.goto('http://localhost:8080/index.html', {
            waitUntil: 'networkidle2'
        });
    }

    async runTests() {
        console.log('🧪 Running browser tests...');

        // Test environment validation
        const envResults = await this.testEnvironment();
        this.testResults.environment = envResults;

        // Test module loading
        const loadResults = await this.testModuleLoading();
        this.testResults.loading = loadResults;

        // Test widget functionality
        const widgetResults = await this.testWidgets();
        this.testResults.widgets = widgetResults;

        // Test performance
        const perfResults = await this.testPerformance();
        this.testResults.performance = perfResults;
    }

    async testEnvironment() {
        console.log('  🔍 Testing environment...');

        const results = await this.page.evaluate(async () => {
            // Wait for environment validation to complete
            await new Promise(resolve => {
                const checkStatus = () => {
                    const status = document.getElementById('env-status');
                    if (status && status.textContent !== 'pending') {
                        resolve();
                    } else {
                        setTimeout(checkStatus, 100);
                    }
                };
                checkStatus();
            });

            // Get environment test results
            const wasmSupport = document.getElementById('wasm-support').textContent;
            const simdSupport = document.getElementById('simd-support').textContent;
            const threadsSupport = document.getElementById('threads-support').textContent;
            const webgpuSupport = document.getElementById('webgpu-support').textContent;
            const sabSupport = document.getElementById('sab-support').textContent;

            return {
                wasmSupport: wasmSupport.includes('✓'),
                simdSupport: simdSupport.includes('✓'),
                threadsSupport: threadsSupport.includes('✓'),
                webgpuSupport: webgpuSupport.includes('✓'),
                sabSupport: sabSupport.includes('✓')
            };
        });

        console.log(`    WebAssembly: ${results.wasmSupport ? '✅' : '❌'}`);
        console.log(`    SIMD: ${results.simdSupport ? '✅' : '❌'}`);
        console.log(`    Threading: ${results.threadsSupport ? '✅' : '❌'}`);
        console.log(`    WebGPU: ${results.webgpuSupport ? '✅' : '❌'}`);
        console.log(`    SharedArrayBuffer: ${results.sabSupport ? '✅' : '❌'}`);

        return results;
    }

    async testModuleLoading() {
        console.log('  📦 Testing module loading...');

        const results = await this.page.evaluate(async () => {
            const loadBtn = document.getElementById('load-btn');
            const loadingStatus = document.getElementById('loading-status');

            // Click load button
            loadBtn.click();

            // Wait for loading to complete (with timeout)
            const startTime = Date.now();
            const timeout = 30000; // 30 seconds

            await new Promise((resolve, reject) => {
                const checkStatus = () => {
                    const status = loadingStatus.textContent;
                    const elapsed = Date.now() - startTime;

                    if (status === 'pass' || status === 'fail') {
                        resolve();
                    } else if (elapsed > timeout) {
                        reject(new Error('Module loading timeout'));
                    } else {
                        setTimeout(checkStatus, 500);
                    }
                };
                checkStatus();
            });

            const success = loadingStatus.textContent === 'pass';
            const log = document.getElementById('loading-log').textContent;

            return {
                success,
                loadTime: Date.now() - startTime,
                log: log.split('\n').slice(-10).join('\n') // Last 10 lines
            };
        });

        console.log(`    Status: ${results.success ? '✅' : '❌'}`);
        console.log(`    Load time: ${results.loadTime}ms`);

        return results;
    }

    async testWidgets() {
        console.log('  🎛️  Testing widgets...');

        const results = await this.page.evaluate(async () => {
            const widgetBtn = document.getElementById('widget-test-btn');
            const widgetStatus = document.getElementById('widget-status');

            if (widgetBtn.disabled) {
                return { success: false, message: 'Widget tests unavailable (module not loaded)' };
            }

            // Click widget test button
            widgetBtn.click();

            const startTime = Date.now();

            // Wait for tests to complete
            await new Promise(resolve => {
                const checkStatus = () => {
                    const status = widgetStatus.textContent;
                    if (status === 'pass' || status === 'fail') {
                        resolve();
                    } else {
                        setTimeout(checkStatus, 500);
                    }
                };
                checkStatus();
            });

            const success = widgetStatus.textContent === 'pass';
            const log = document.getElementById('widget-log').textContent;
            const testTime = Date.now() - startTime;

            // Parse test results from log
            const logLines = log.split('\n');
            const passedCount = logLines.filter(line => line.includes('✓')).length;
            const failedCount = logLines.filter(line => line.includes('✗')).length;

            return {
                success,
                testTime,
                passedCount,
                failedCount,
                totalTests: passedCount + failedCount
            };
        });

        console.log(`    Status: ${results.success ? '✅' : '❌'}`);
        console.log(`    Tests: ${results.passedCount}/${results.totalTests} passed`);
        console.log(`    Duration: ${results.testTime}ms`);

        return results;
    }

    async testPerformance() {
        console.log('  ⚡ Testing performance...');

        const results = await this.page.evaluate(async () => {
            const benchmarkBtn = document.getElementById('benchmark-btn');
            const perfStatus = document.getElementById('perf-status');

            if (benchmarkBtn.disabled) {
                return { success: false, message: 'Benchmarks unavailable (module not loaded)' };
            }

            // Click benchmark button
            benchmarkBtn.click();

            const startTime = Date.now();

            // Wait for benchmarks to complete
            await new Promise(resolve => {
                const checkStatus = () => {
                    const status = perfStatus.textContent;
                    if (status === 'pass' || status === 'fail') {
                        resolve();
                    } else {
                        setTimeout(checkStatus, 1000);
                    }
                };
                checkStatus();
            });

            const success = perfStatus.textContent === 'pass';
            const benchmarkTime = Date.now() - startTime;

            // Extract benchmark results
            const benchmarkCards = Array.from(document.querySelectorAll('.benchmark-card'));
            const benchmarks = benchmarkCards.map(card => {
                const title = card.querySelector('h4').textContent;
                const metrics = {};
                
                card.querySelectorAll('.metric').forEach(metric => {
                    const key = metric.querySelector('span:first-child').textContent.replace(':', '');
                    const value = metric.querySelector('.metric-value').textContent;
                    metrics[key] = value;
                });

                return { name: title, metrics };
            });

            return {
                success,
                benchmarkTime,
                benchmarks
            };
        });

        console.log(`    Status: ${results.success ? '✅' : '❌'}`);
        console.log(`    Benchmark time: ${results.benchmarkTime}ms`);
        console.log(`    Benchmarks run: ${results.benchmarks ? results.benchmarks.length : 0}`);

        return results;
    }

    async generateReport() {
        console.log('\n📊 Browser Test Report');
        console.log('=' .repeat(50));

        const report = {
            timestamp: new Date().toISOString(),
            browser: await this.page.browser().version(),
            userAgent: await this.page.evaluate(() => navigator.userAgent),
            results: this.testResults
        };

        // Calculate overall success
        const categories = Object.keys(this.testResults);
        const successfulCategories = categories.filter(cat => 
            this.testResults[cat].success !== false
        );

        console.log(`Browser: ${report.browser}`);
        console.log(`Categories: ${successfulCategories.length}/${categories.length} passed`);

        categories.forEach(category => {
            const result = this.testResults[category];
            console.log(`  ${category}: ${result.success ? '✅' : '❌'}`);
        });

        // Save detailed report
        const reportPath = path.join(__dirname, 'browser-test-report.json');
        fs.writeFileSync(reportPath, JSON.stringify(report, null, 2));
        console.log(`\nDetailed report saved: ${reportPath}`);

        // Exit with appropriate code
        const allPassed = successfulCategories.length === categories.length;
        if (allPassed) {
            console.log('🎉 All browser tests passed!');
        } else {
            console.log('❌ Some browser tests failed');
            process.exit(1);
        }
    }

    async cleanup() {
        if (this.browser) {
            await this.browser.close();
        }
    }
}

// Run tests if called directly
if (require.main === module) {
    const runner = new BrowserTestRunner();
    runner.run().catch(error => {
        console.error('Browser test runner failed:', error);
        process.exit(1);
    });
}

module.exports = BrowserTestRunner;