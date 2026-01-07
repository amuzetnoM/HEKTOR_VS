import { Component, ElementRef, ViewChild, signal, OnDestroy, AfterViewInit, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { VectorDbService } from '../services/vector-db.service';
import { Subscription } from 'rxjs';

declare const d3: any;

@Component({
  selector: 'app-health-monitor',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="h-full w-full p-6 md:p-10 overflow-y-auto custom-scrollbar pt-16 md:pt-10">
      <div class="max-w-7xl mx-auto space-y-10">
        <!-- Header -->
        <div class="flex flex-col md:flex-row md:items-end justify-between border-b border-white/5 pb-8 gap-4">
          <div>
            <h2 class="text-3xl font-bold text-white tracking-tight">System Overview</h2>
            <p class="text-zinc-400 mt-2 font-light text-lg">Real-time cluster telemetry and performance metrics.</p>
          </div>
          <div class="flex items-center gap-2">
             <div class="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></div>
             <div class="text-sm font-mono text-zinc-500">LIVE STREAM</div>
          </div>
        </div>

        <!-- Top Cards -->
        <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-4 gap-6">
          <div class="bg-white/[0.02] border border-white/5 p-6 rounded-2xl hover:bg-white/[0.04] transition-colors group">
             <div class="flex justify-between items-start mb-4">
               <div class="text-[10px] font-bold text-zinc-500 uppercase tracking-widest">System Status</div>
               <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 text-emerald-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" /></svg>
             </div>
             <div class="text-2xl font-bold text-white">Active</div>
             <div class="mt-2 text-xs text-emerald-400 font-mono">Gemini API Connected</div>
          </div>
           <div class="bg-white/[0.02] border border-white/5 p-6 rounded-2xl hover:bg-white/[0.04] transition-colors">
             <div class="flex justify-between items-start mb-4">
               <div class="text-[10px] font-bold text-zinc-500 uppercase tracking-widest">Total Docs</div>
               <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 text-zinc-600" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10" /></svg>
             </div>
             <div class="text-3xl font-mono text-white tracking-tighter">{{ db.stats().totalDocs }}</div>
          </div>
           <div class="bg-white/[0.02] border border-white/5 p-6 rounded-2xl hover:bg-white/[0.04] transition-colors">
             <div class="flex justify-between items-start mb-4">
               <div class="text-[10px] font-bold text-zinc-500 uppercase tracking-widest">Collections</div>
               <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 text-zinc-600" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 11H5m14 0a2 2 0 012 2v6a2 2 0 01-2 2H5a2 2 0 01-2-2v-6a2 2 0 012-2m14 0V9a2 2 0 00-2-2M5 11V9a2 2 0 012-2m0 0V5a2 2 0 012-2h6a2 2 0 012 2v2M7 7h10" /></svg>
             </div>
             <div class="text-3xl font-mono text-white tracking-tighter">{{ db.collections().length }}</div>
          </div>
           <div class="bg-white/[0.02] border border-white/5 p-6 rounded-2xl hover:bg-white/[0.04] transition-colors">
             <div class="flex justify-between items-start mb-4">
               <div class="text-[10px] font-bold text-zinc-500 uppercase tracking-widest">Memory Est.</div>
               <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 text-zinc-600" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z" /></svg>
             </div>
             <div class="text-3xl font-mono text-indigo-400 tracking-tighter">{{ db.stats().memoryUsage }}</div>
          </div>
        </div>

        @if (db.collections().length === 0) {
            <div class="p-12 border border-dashed border-zinc-800 rounded-2xl flex flex-col items-center justify-center text-zinc-500">
               <svg xmlns="http://www.w3.org/2000/svg" class="h-12 w-12 mb-4 opacity-50" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M20 13V6a2 2 0 00-2-2H6a2 2 0 00-2 2v7m16 0v5a2 2 0 01-2 2H6a2 2 0 01-2-2v-5m16 0h-2.586a1 1 0 00-.707.293l-2.414 2.414a1 1 0 01-.707.293h-3.172a1 1 0 01-.707-.293l-2.414-2.414A1 1 0 006.586 13H4" /></svg>
               <h3 class="text-lg font-medium text-white">Database is Empty</h3>
               <p class="text-sm max-w-sm text-center mt-2">Create a collection via the Sidebar or ask the AI Agent to "Create a knowledge base".</p>
            </div>
        } @else {
            <!-- Charts Row -->
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-8">
            <!-- Latency Chart -->
            <div class="bg-black/20 border border-white/5 rounded-2xl p-6 h-[300px] md:h-[400px] flex flex-col relative overflow-hidden ring-1 ring-white/5">
                <div class="flex justify-between items-center mb-6 z-10 relative">
                <h3 class="text-sm font-bold text-zinc-300 flex items-center gap-2 uppercase tracking-wide">
                    <span class="w-2 h-2 bg-blue-500 rounded-full shadow-[0_0_10px_rgba(59,130,246,0.5)]"></span>
                    API Latency (ms)
                </h3>
                <div class="text-xs font-mono text-zinc-500">LIVE</div>
                </div>
                <div #latencyChart class="flex-1 w-full min-h-0 z-10"></div>
                <div class="absolute inset-0 opacity-[0.03]" style="background-image: linear-gradient(#4f46e5 1px, transparent 1px), linear-gradient(90deg, #4f46e5 1px, transparent 1px); background-size: 40px 40px;"></div>
            </div>

            <!-- Throughput Chart -->
            <div class="bg-black/20 border border-white/5 rounded-2xl p-6 h-[300px] md:h-[400px] flex flex-col relative overflow-hidden ring-1 ring-white/5">
                <div class="flex justify-between items-center mb-6 z-10 relative">
                <h3 class="text-sm font-bold text-zinc-300 flex items-center gap-2 uppercase tracking-wide">
                    <span class="w-2 h-2 bg-indigo-500 rounded-full shadow-[0_0_10px_rgba(99,102,241,0.5)]"></span>
                    Ops Throughput
                </h3>
                <div class="text-xs font-mono text-zinc-500">OPS/BATCH</div>
                </div>
                <div #tpsChart class="flex-1 w-full min-h-0 z-10"></div>
                <div class="absolute inset-0 opacity-[0.03]" style="background-image: linear-gradient(#8b5cf6 1px, transparent 1px), linear-gradient(90deg, #8b5cf6 1px, transparent 1px); background-size: 40px 40px;"></div>
            </div>
            </div>
        }
      </div>
    </div>
  `
})
export class HealthMonitorComponent implements AfterViewInit, OnDestroy {
  db = inject(VectorDbService);
  
  @ViewChild('latencyChart') latencyContainer!: ElementRef;
  @ViewChild('tpsChart') tpsContainer!: ElementRef;

  private sub: Subscription | null = null;
  private intervalId: any;
  private latencyData: { time: number, value: number }[] = [];
  private tpsData: { time: number, value: number }[] = [];
  private maxPoints = 30;

  ngAfterViewInit() {
    this.initEmptyCharts();
    
    // Subscribe to real events
    this.sub = this.db.telemetry$.subscribe(event => {
        const now = Date.now();
        if (event.type === 'latency') {
            this.latencyData.push({ time: now, value: event.value });
            if (this.latencyData.length > this.maxPoints) this.latencyData.shift();
            this.renderCharts();
        } else if (event.type === 'throughput') {
            this.tpsData.push({ time: now, value: event.value });
            if (this.tpsData.length > this.maxPoints) this.tpsData.shift();
            this.renderCharts();
        }
    });

    // Keep chart moving even if idle
    this.intervalId = setInterval(() => {
        const now = Date.now();
        // Push 0 if no recent activity to show "heartbeat"
        const lastLat = this.latencyData[this.latencyData.length - 1];
        if (lastLat && now - lastLat.time > 2000) {
            this.latencyData.push({ time: now, value: 0 });
            if (this.latencyData.length > this.maxPoints) this.latencyData.shift();
        }
        
        const lastTps = this.tpsData[this.tpsData.length - 1];
        if (lastTps && now - lastTps.time > 2000) {
            this.tpsData.push({ time: now, value: 0 });
            if (this.tpsData.length > this.maxPoints) this.tpsData.shift();
        }
        
        this.renderCharts();
    }, 1000);

    window.addEventListener('resize', () => this.renderCharts());
  }

  ngOnDestroy() {
    this.sub?.unsubscribe();
    if (this.intervalId) clearInterval(this.intervalId);
    window.removeEventListener('resize', () => this.renderCharts());
  }

  private initEmptyCharts() {
      const now = Date.now();
      for (let i = 0; i < this.maxPoints; i++) {
          this.latencyData.push({ time: now - (this.maxPoints - i) * 1000, value: 0 });
          this.tpsData.push({ time: now - (this.maxPoints - i) * 1000, value: 0 });
      }
      setTimeout(() => this.renderCharts(), 0);
  }

  private renderCharts() {
    if (this.db.collections().length === 0) return;
    this.drawChart(this.latencyContainer?.nativeElement, this.latencyData, '#3b82f6');
    this.drawChart(this.tpsContainer?.nativeElement, this.tpsData, '#8b5cf6');
  }

  private drawChart(container: HTMLElement, data: any[], color: string) {
    if (!container) return;
    d3.select(container).selectAll('*').remove();

    const width = container.clientWidth;
    const height = container.clientHeight;
    const margin = { top: 10, right: 0, bottom: 20, left: 0 };

    const svg = d3.select(container)
      .append('svg')
      .attr('width', width)
      .attr('height', height);

    const x = d3.scaleTime()
      .domain(d3.extent(data, (d: any) => d.time))
      .range([margin.left, width - margin.right]);

    const maxVal = d3.max(data, (d: any) => d.value) || 10;
    const y = d3.scaleLinear()
      .domain([0, maxVal * 1.2])
      .range([height - margin.bottom, margin.top]);

    const line = d3.line()
      .x((d: any) => x(d.time))
      .y((d: any) => y(d.value))
      .curve(d3.curveMonotoneX);

    const area = d3.area()
      .x((d: any) => x(d.time))
      .y0(height - margin.bottom)
      .y1((d: any) => y(d.value))
      .curve(d3.curveMonotoneX);

    const defs = svg.append("defs");
    const gradient = defs.append("linearGradient")
      .attr("id", "grad-" + color.replace('#', ''))
      .attr("x1", "0%").attr("y1", "0%")
      .attr("x2", "0%").attr("y2", "100%");
    
    gradient.append("stop").attr("offset", "0%").attr("stop-color", color).attr("stop-opacity", 0.4);
    gradient.append("stop").attr("offset", "100%").attr("stop-color", color).attr("stop-opacity", 0);

    const filter = defs.append("filter").attr("id", "glow");
    filter.append("feGaussianBlur").attr("stdDeviation", "2").attr("result", "coloredBlur");
    const feMerge = filter.append("feMerge");
    feMerge.append("feMergeNode").attr("in", "coloredBlur");
    feMerge.append("feMergeNode").attr("in", "SourceGraphic");

    svg.append("path")
      .datum(data)
      .attr("fill", `url(#grad-${color.replace('#', '')})`)
      .attr("d", area);

    svg.append("path")
      .datum(data)
      .attr("fill", "none")
      .attr("stroke", color)
      .attr("stroke-width", 2)
      .attr("d", line)
      .style("filter", "url(#glow)");
  }
}