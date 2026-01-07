import { Component, input, output, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { IngestionConfig } from '../models/core';

@Component({
  selector: 'app-ingestion-wizard',
  standalone: true,
  imports: [CommonModule, FormsModule],
  template: `
    <div class="fixed inset-0 bg-black/80 backdrop-blur-sm z-50 flex items-center justify-center p-4">
      <div class="bg-surface border border-border w-full max-w-2xl rounded-xl shadow-2xl flex flex-col max-h-[90vh]">
        
        <!-- Header -->
        <div class="px-6 py-4 border-b border-border flex justify-between items-center">
          <div>
            <h2 class="text-lg font-bold text-white">Import Data</h2>
            <p class="text-xs text-muted">Ingest files into <span class="text-primary font-mono">{{ collectionName() }}</span></p>
          </div>
          <button (click)="close.emit()" class="text-muted hover:text-white">
            <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" /></svg>
          </button>
        </div>

        <!-- Content -->
        <div class="p-6 overflow-y-auto space-y-6 custom-scrollbar">
          
          <!-- Drop Zone -->
          <div 
            (dragover)="onDragOver($event)"
            (dragleave)="onDragLeave($event)"
            (drop)="onDrop($event)"
            class="border-2 border-dashed rounded-xl p-8 flex flex-col items-center justify-center transition-all cursor-pointer group"
            [class.border-primary]="isDragging()"
            [class.bg-primary]="isDragging()"
            [class.bg-opacity-5]="isDragging()"
            [class.border-zinc-700]="!isDragging()"
            [class.bg-zinc-900]="!isDragging()"
            (click)="fileInput.click()"
          >
            <input #fileInput type="file" multiple class="hidden" (change)="onFileSelected($event)" />
            
            <div class="w-12 h-12 rounded-full bg-zinc-800 flex items-center justify-center mb-4 group-hover:bg-primary/20 group-hover:text-primary transition-colors text-zinc-400">
               <svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12" /></svg>
            </div>
            <p class="text-sm font-medium text-zinc-300">Click to upload or drag and drop</p>
            <p class="text-xs text-zinc-500 mt-1">PDF, TXT, JSON, CSV (Max 10MB)</p>
          </div>

          <!-- File List -->
          @if (files().length > 0) {
            <div class="space-y-2">
               @for (file of files(); track file.name) {
                 <div class="flex items-center justify-between p-2 bg-black/20 rounded border border-border text-xs">
                    <span class="text-zinc-300 truncate">{{ file.name }}</span>
                    <button (click)="removeFile(file)" class="text-red-400 hover:text-red-300">Remove</button>
                 </div>
               }
            </div>
          }

          <!-- Configuration -->
          <div class="grid grid-cols-2 gap-6">
             <div class="space-y-2">
               <label class="text-xs font-bold text-muted uppercase">Chunk Size (Tokens)</label>
               <input type="number" [(ngModel)]="chunkSize" class="w-full bg-black/20 border border-border rounded px-3 py-2 text-sm text-white focus:border-primary focus:outline-none" />
               <p class="text-[10px] text-zinc-600">Optimal: 512 for standard, 1024 for complex context.</p>
             </div>
             <div class="space-y-2">
               <label class="text-xs font-bold text-muted uppercase">Chunk Overlap</label>
               <input type="number" [(ngModel)]="chunkOverlap" class="w-full bg-black/20 border border-border rounded px-3 py-2 text-sm text-white focus:border-primary focus:outline-none" />
               <p class="text-[10px] text-zinc-600">Usually 10-20% of chunk size.</p>
             </div>
          </div>

          <div class="space-y-2">
             <label class="text-xs font-bold text-muted uppercase">Split Strategy</label>
             <div class="flex gap-2">
                <button (click)="strategy.set('recursive')" [class.bg-primary]="strategy() === 'recursive'" [class.bg-zinc-800]="strategy() !== 'recursive'" class="flex-1 py-2 rounded text-xs font-medium text-white border border-transparent transition-colors">Recursive</button>
                <button (click)="strategy.set('fixed')" [class.bg-primary]="strategy() === 'fixed'" [class.bg-zinc-800]="strategy() !== 'fixed'" class="flex-1 py-2 rounded text-xs font-medium text-white border border-transparent transition-colors">Fixed Size</button>
                <button (click)="strategy.set('semantic')" [class.bg-primary]="strategy() === 'semantic'" [class.bg-zinc-800]="strategy() !== 'semantic'" class="flex-1 py-2 rounded text-xs font-medium text-white border border-transparent transition-colors">Semantic</button>
             </div>
          </div>
        </div>

        <!-- Footer -->
        <div class="p-6 border-t border-border bg-zinc-900/50 flex justify-end gap-3">
           <button (click)="close.emit()" class="px-4 py-2 rounded text-sm font-medium text-zinc-400 hover:text-white transition-colors">Cancel</button>
           <button 
             (click)="startIngestion()" 
             [disabled]="files().length === 0 || isProcessing()"
             class="px-4 py-2 rounded text-sm font-medium bg-primary hover:bg-primaryHover text-white disabled:opacity-50 disabled:cursor-not-allowed flex items-center gap-2">
             @if (isProcessing()) {
               <div class="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
             }
             Start Ingestion
           </button>
        </div>
      </div>
    </div>
  `
})
export class IngestionWizardComponent {
  collectionName = input.required<string>();
  close = output<void>();
  ingest = output<IngestionConfig>();

  files = signal<File[]>([]);
  isDragging = signal(false);
  isProcessing = signal(false);
  
  // Config state
  chunkSize = signal(512);
  chunkOverlap = signal(50);
  strategy = signal<'recursive' | 'fixed' | 'semantic'>('recursive');

  onDragOver(e: DragEvent) {
    e.preventDefault();
    this.isDragging.set(true);
  }

  onDragLeave(e: DragEvent) {
    e.preventDefault();
    this.isDragging.set(false);
  }

  onDrop(e: DragEvent) {
    e.preventDefault();
    this.isDragging.set(false);
    if (e.dataTransfer?.files) {
      this.addFiles(e.dataTransfer.files);
    }
  }

  onFileSelected(e: Event) {
    const input = e.target as HTMLInputElement;
    if (input.files) {
      this.addFiles(input.files);
    }
  }

  addFiles(fileList: FileList) {
    const newFiles = Array.from(fileList);
    this.files.update(f => [...f, ...newFiles]);
  }

  removeFile(file: File) {
    this.files.update(f => f.filter(x => x !== file));
  }

  async startIngestion() {
    this.isProcessing.set(true);
    const config: IngestionConfig = {
      collectionName: this.collectionName(),
      files: this.files(),
      chunkSize: this.chunkSize(),
      chunkOverlap: this.chunkOverlap(),
      strategy: this.strategy()
    };
    
    // Emit config to parent to handle service call
    this.ingest.emit(config);
  }
}