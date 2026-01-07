import { Component, input, signal, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VectorDbService } from '../services/vector-db.service';
import { VectorDoc } from '../models/core';

@Component({
  selector: 'app-playground',
  standalone: true,
  imports: [CommonModule, FormsModule],
  template: `
    <div class="h-full flex flex-col md:flex-row overflow-hidden bg-[#09090b]">
      
      <!-- Controls Pane -->
      <div class="w-full md:w-80 border-b md:border-b-0 md:border-r border-border bg-surface/30 p-4 flex flex-col gap-6 overflow-y-auto custom-scrollbar z-10">
        <div>
           <h3 class="text-xs font-bold text-muted uppercase tracking-widest mb-4">Search Parameters</h3>
           
           <div class="space-y-4">
              <div>
                <div class="flex justify-between text-xs mb-1">
                   <label class="text-zinc-300">Top K</label>
                   <span class="font-mono text-primary">{{ topK() }}</span>
                </div>
                <input type="range" min="1" max="20" [(ngModel)]="topK" class="w-full h-1 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-primary">
              </div>

              <div>
                <div class="flex justify-between text-xs mb-1">
                   <label class="text-zinc-300">Min Score Threshold</label>
                   <span class="font-mono text-primary">{{ minScore() }}</span>
                </div>
                <input type="range" min="0" max="1" step="0.05" [(ngModel)]="minScore" class="w-full h-1 bg-zinc-700 rounded-lg appearance-none cursor-pointer accent-primary">
              </div>
           </div>
        </div>

        <div class="border-t border-border pt-4">
           <h3 class="text-xs font-bold text-muted uppercase tracking-widest mb-4">Metadata Filters</h3>
           
           <div class="space-y-2">
              <div class="flex gap-2">
                 <input #filterKey placeholder="Key" class="w-1/2 bg-black/20 border border-border rounded px-2 py-1.5 text-xs text-white focus:border-primary focus:outline-none">
                 <input #filterVal placeholder="Value" class="w-1/2 bg-black/20 border border-border rounded px-2 py-1.5 text-xs text-white focus:border-primary focus:outline-none">
                 <button (click)="addFilter(filterKey.value, filterVal.value); filterKey.value=''; filterVal.value=''" class="bg-primary/20 hover:bg-primary/40 text-primary p-1.5 rounded transition-colors">
                    <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 4v16m8-8H4" /></svg>
                 </button>
              </div>

              <div class="flex flex-wrap gap-2 mt-2">
                 @for (item of filters() | keyvalue; track item.key) {
                    <span class="inline-flex items-center gap-1 px-2 py-1 rounded bg-zinc-800 border border-zinc-700 text-[10px] text-zinc-300">
                       <span class="font-semibold">{{ item.key }}:</span> {{ item.value }}
                       <button (click)="removeFilter(item.key)" class="hover:text-red-400"><svg xmlns="http://www.w3.org/2000/svg" class="h-3 w-3" viewBox="0 0 20 20" fill="currentColor"><path fill-rule="evenodd" d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z" clip-rule="evenodd" /></svg></button>
                    </span>
                 }
              </div>
           </div>
        </div>
      </div>

      <!-- Execution Pane -->
      <div class="flex-1 flex flex-col min-w-0">
         
         <!-- Prompt Input -->
         <div class="p-4 border-b border-border bg-surface/10">
            <label class="text-xs font-bold text-muted uppercase tracking-widest mb-2 block">Test Prompt</label>
            <div class="relative">
               <textarea 
                  [(ngModel)]="query" 
                  (keydown.control.enter)="runSearch()"
                  placeholder="Enter semantic query..."
                  class="w-full bg-black/20 border border-border rounded-lg p-3 text-sm text-zinc-200 focus:outline-none focus:border-primary/50 focus:ring-1 focus:ring-primary/20 min-h-[80px] resize-none font-sans"
               ></textarea>
               <button 
                  (click)="runSearch()" 
                  [disabled]="isSearching()"
                  class="absolute bottom-3 right-3 px-3 py-1.5 bg-primary hover:bg-primaryHover text-white text-xs font-bold rounded shadow-lg shadow-primary/20 transition-all disabled:opacity-50 flex items-center gap-2">
                  @if (isSearching()) { <div class="w-3 h-3 border-2 border-white/30 border-t-white rounded-full animate-spin"></div> }
                  RUN
               </button>
            </div>
            <p class="text-[10px] text-zinc-500 mt-2 text-right">Ctrl + Enter to run</p>
         </div>

         <!-- Results -->
         <div class="flex-1 overflow-y-auto p-4 space-y-4 custom-scrollbar bg-black/40">
            @if (results().length === 0 && !hasSearched()) {
               <div class="h-full flex flex-col items-center justify-center text-zinc-600">
                  <svg xmlns="http://www.w3.org/2000/svg" class="h-12 w-12 mb-4 opacity-20" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" /></svg>
                  <p class="text-sm font-medium">Run a query to inspect vector results</p>
               </div>
            }

            @for (res of results(); track res.id) {
               <div class="bg-surface border border-border rounded-lg p-4 hover:border-zinc-600 transition-colors animate-in fade-in slide-in-from-bottom-2 duration-300">
                  <div class="flex justify-between items-start mb-2">
                     <span class="font-mono text-[10px] bg-zinc-800 px-1.5 py-0.5 rounded text-zinc-400 select-all">{{ res.id }}</span>
                     <span class="font-mono text-xs font-bold" [class.text-emerald-400]="res.score! > 0.8" [class.text-yellow-400]="res.score! <= 0.8">{{ (res.score! * 100).toFixed(1) }}%</span>
                  </div>
                  <p class="text-sm text-zinc-300 leading-relaxed mb-3">{{ res.content }}</p>
                  
                  <details class="group">
                     <summary class="text-[10px] font-bold text-muted uppercase tracking-widest cursor-pointer list-none flex items-center gap-1">
                        <svg xmlns="http://www.w3.org/2000/svg" class="h-3 w-3 transition-transform group-open:rotate-90" viewBox="0 0 20 20" fill="currentColor"><path fill-rule="evenodd" d="M7.293 14.707a1 1 0 010-1.414L10.586 10 7.293 6.707a1 1 0 011.414-1.414l4 4a1 1 0 010 1.414l-4 4a1 1 0 01-1.414 0z" clip-rule="evenodd" /></svg>
                        JSON Metadata
                     </summary>
                     <pre class="mt-2 p-2 bg-black/50 rounded text-[10px] text-zinc-400 font-mono overflow-x-auto">{{ res.metadata | json }}</pre>
                  </details>
               </div>
            }
         </div>
      </div>
    </div>
  `
})
export class PlaygroundComponent {
  collectionName = input.required<string>();
  db = inject(VectorDbService);

  query = signal('');
  topK = signal(5);
  minScore = signal(0.7);
  filters = signal<Record<string, any>>({});
  
  results = signal<VectorDoc[]>([]);
  isSearching = signal(false);
  hasSearched = signal(false);

  addFilter(key: string, val: string) {
    if (!key) return;
    this.filters.update(f => ({ ...f, [key]: val }));
  }

  removeFilter(key: string) {
    this.filters.update(f => {
       const n = { ...f };
       delete n[key];
       return n;
    });
  }

  async runSearch() {
    if (!this.query().trim()) return;
    
    this.isSearching.set(true);
    this.hasSearched.set(true);
    
    try {
       const res = await this.db.query(this.collectionName(), {
          query: this.query(),
          topK: Number(this.topK()),
          minScore: Number(this.minScore()),
          filters: this.filters()
       });
       this.results.set(res);
    } finally {
       this.isSearching.set(false);
    }
  }
}