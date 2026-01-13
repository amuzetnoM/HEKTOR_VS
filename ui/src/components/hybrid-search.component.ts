import { Component, inject, signal, Input } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VectorDbService } from '../services/vector-db.service';
import { HektorApiService } from '../services/hektor-api.service';
import { FusionMethod, HybridResult } from '../models/core';
import { environment } from '../environments/environment';

@Component({
    selector: 'app-hybrid-search',
    standalone: true,
    imports: [CommonModule, FormsModule],
    template: `
    <div class="h-full flex flex-col bg-[#09090b] overflow-hidden">
      <!-- Header -->
      <div class="border-b border-white/5 p-4 sm:p-6 bg-[#09090b] flex-shrink-0">
        <h2 class="text-lg sm:text-xl font-bold text-white mb-1">Hybrid Search</h2>
        <p class="text-xs sm:text-sm text-zinc-500">Combine vector similarity with BM25 full-text search</p>
      </div>

      <!-- Controls -->
      <div class="p-4 sm:p-6 border-b border-white/5 space-y-4 sm:space-y-6 flex-shrink-0 overflow-y-auto custom-scrollbar">
        <!-- Query Input -->
        <div>
          <label class="block text-[10px] sm:text-xs font-bold text-zinc-500 uppercase tracking-wider mb-1.5 sm:mb-2">Query</label>
          <div class="relative">
            <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4 sm:h-5 sm:w-5 absolute left-2.5 sm:left-3 top-2.5 sm:top-3 text-zinc-500" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
            </svg>
            <input 
              type="text" 
              [(ngModel)]="query"
              (keyup.enter)="executeSearch()"
              placeholder="Enter your search query..."
              class="w-full bg-zinc-900/50 border border-white/10 rounded-lg pl-9 sm:pl-11 pr-3 sm:pr-4 py-2 sm:py-3 text-sm sm:text-base text-white focus:outline-none focus:border-indigo-500/50 focus:ring-1 focus:ring-indigo-500/20 transition-all placeholder-zinc-600"
            />
          </div>
        </div>

        <div class="grid grid-cols-2 lg:grid-cols-4 gap-3 sm:gap-4">
          <!-- Fusion Method -->
          <div>
            <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Fusion Method</label>
            <select 
              [(ngModel)]="fusionMethod"
              class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
            >
              <option value="rrf">RRF (Reciprocal Rank Fusion)</option>
              <option value="weighted">Weighted Sum</option>
              <option value="combsum">CombSUM</option>
              <option value="combmnz">CombMNZ</option>
              <option value="borda">Borda Count</option>
            </select>
          </div>

          <!-- Top K -->
          <div>
            <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Results (Top K)</label>
            <input 
              type="number" 
              [(ngModel)]="topK"
              min="1" 
              max="100"
              class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
            />
          </div>

          <!-- Vector Weight -->
          <div>
            <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">
              Vector Weight: {{ vectorWeight.toFixed(2) }}
            </label>
            <input 
              type="range" 
              [(ngModel)]="vectorWeight"
              min="0" 
              max="1" 
              step="0.05"
              class="w-full accent-indigo-500"
            />
          </div>

          <!-- Lexical Weight -->
          <div>
            <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">
              Lexical Weight: {{ lexicalWeight.toFixed(2) }}
            </label>
            <input 
              type="range" 
              [(ngModel)]="lexicalWeight"
              min="0" 
              max="1" 
              step="0.05"
              class="w-full accent-emerald-500"
            />
          </div>
        </div>

        <!-- Search Button -->
        <div class="flex justify-end">
          <button 
            (click)="executeSearch()"
            [disabled]="isSearching() || !query.trim()"
            class="px-6 py-2.5 bg-indigo-600 hover:bg-indigo-500 disabled:bg-zinc-800 disabled:text-zinc-500 text-white text-sm font-medium rounded-lg transition-colors flex items-center gap-2"
          >
            @if (isSearching()) {
              <svg class="animate-spin h-4 w-4" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"></circle>
                <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
              </svg>
              Searching...
            } @else {
              <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
              </svg>
              Search
            }
          </button>
        </div>
      </div>

      <!-- Results -->
      <div class="flex-1 overflow-auto custom-scrollbar p-6">
        @if (results().length === 0 && !isSearching()) {
          <div class="flex flex-col items-center justify-center h-full text-zinc-600">
            <svg xmlns="http://www.w3.org/2000/svg" class="h-16 w-16 mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1" d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
            </svg>
            <p class="text-sm">Enter a query to search</p>
          </div>
        } @else {
          <!-- Stats Bar -->
          @if (lastSearchTime()) {
            <div class="mb-4 flex items-center justify-between text-xs text-zinc-500">
              <span>Found {{ results().length }} results</span>
              <span>{{ lastSearchTime() }}ms</span>
            </div>
          }

          <!-- Results Table -->
          <div class="rounded-xl border border-white/5 overflow-hidden">
            <table class="w-full text-left text-sm">
              <thead class="bg-zinc-900/50 text-zinc-500 uppercase text-xs tracking-wider">
                <tr>
                  <th class="px-4 py-3 font-medium">Content</th>
                  <th class="px-4 py-3 font-medium w-24 text-center">Combined</th>
                  <th class="px-4 py-3 font-medium w-24 text-center">Vector</th>
                  <th class="px-4 py-3 font-medium w-24 text-center">Lexical</th>
                  <th class="px-4 py-3 font-medium w-48">Keywords</th>
                </tr>
              </thead>
              <tbody class="divide-y divide-white/5">
                @for (result of results(); track result.id) {
                  <tr class="hover:bg-white/[0.02] transition-colors">
                    <td class="px-4 py-4">
                      <div class="text-zinc-300 line-clamp-2">{{ result.content }}</div>
                      <div class="text-xs text-zinc-600 mt-1 font-mono">{{ result.id }}</div>
                    </td>
                    <td class="px-4 py-4 text-center">
                      <span class="inline-flex items-center justify-center px-2 py-1 rounded bg-indigo-500/10 text-indigo-400 text-xs font-mono">
                        {{ result.combinedScore.toFixed(3) }}
                      </span>
                    </td>
                    <td class="px-4 py-4 text-center">
                      <span class="text-xs font-mono text-zinc-400">{{ result.vectorScore.toFixed(3) }}</span>
                    </td>
                    <td class="px-4 py-4 text-center">
                      <span class="text-xs font-mono text-zinc-400">{{ result.lexicalScore.toFixed(3) }}</span>
                    </td>
                    <td class="px-4 py-4">
                      <div class="flex flex-wrap gap-1">
                        @for (keyword of result.matchedKeywords; track keyword) {
                          <span class="px-1.5 py-0.5 bg-emerald-500/10 text-emerald-400 text-xs rounded">
                            {{ keyword }}
                          </span>
                        }
                      </div>
                    </td>
                  </tr>
                }
              </tbody>
            </table>
          </div>
        }
      </div>
    </div>
  `
})
export class HybridSearchComponent {
    @Input() collectionName = '';

    private db = inject(VectorDbService);
    private api = inject(HektorApiService);
    private useBackend = environment.useBackend;

    query = '';
    fusionMethod: FusionMethod = 'rrf';
    topK = 10;
    vectorWeight = 0.5;
    lexicalWeight = 0.5;

    isSearching = signal(false);
    results = signal<HybridResult[]>([]);
    lastSearchTime = signal<number | null>(null);

    async executeSearch() {
        if (!this.query.trim() || !this.collectionName) return;

        this.isSearching.set(true);
        const start = Date.now();

        try {
            let searchResults: HybridResult[];
            
            if (this.useBackend) {
                // Use real backend API
                searchResults = await this.api.hybridSearch({
                    query: this.query,
                    topK: this.topK,
                    fusionMethod: this.fusionMethod,
                    vectorWeight: this.vectorWeight,
                    lexicalWeight: this.lexicalWeight
                });
            } else {
                // Use in-memory database with mock hybrid search
                searchResults = await this.db.hybridSearch(this.collectionName, {
                    query: this.query,
                    topK: this.topK,
                    fusionMethod: this.fusionMethod,
                    vectorWeight: this.vectorWeight,
                    lexicalWeight: this.lexicalWeight
                });
            }

            this.results.set(searchResults);
            this.lastSearchTime.set(Date.now() - start);
            
            // Report telemetry
            this.db.telemetry$.next({ type: 'latency', value: Date.now() - start });
        } catch (error) {
            console.error('Hybrid search failed:', error);
            this.results.set([]);
        } finally {
            this.isSearching.set(false);
        }
    }
}
