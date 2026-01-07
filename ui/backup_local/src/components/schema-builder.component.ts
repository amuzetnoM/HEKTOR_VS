import { Component, input, output, signal, effect } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { SchemaField } from '../models/core';

@Component({
  selector: 'app-schema-builder',
  standalone: true,
  imports: [CommonModule, FormsModule],
  template: `
    <div class="bg-surface border border-border rounded-lg p-6 shadow-sm max-w-3xl mx-auto">
      <div class="flex justify-between items-center mb-6 border-b border-border pb-4">
        <div>
          <h3 class="text-base font-semibold text-white">Metadata Schema</h3>
          <p class="text-xs text-muted mt-1">Define properties to enforce type safety on upsert.</p>
        </div>
        <button (click)="saveSchema()" class="px-3 py-1.5 bg-primary hover:bg-primaryHover text-white rounded text-xs font-medium transition-colors shadow-lg shadow-primary/20">
          Save Schema
        </button>
      </div>

      <div class="space-y-2">
        <!-- Headers -->
        <div class="grid grid-cols-12 gap-3 text-[10px] font-bold text-muted uppercase px-3 pb-1 tracking-wider">
          <div class="col-span-5">Field Name</div>
          <div class="col-span-3">Type</div>
          <div class="col-span-2">Required</div>
          <div class="col-span-2 text-right"></div>
        </div>

        <!-- Rows -->
        @for (field of localSchema(); track $index) {
          <div class="grid grid-cols-12 gap-3 items-center bg-black/20 p-2 rounded border border-border group hover:border-zinc-700 transition-colors">
            <div class="col-span-5">
              <input 
                [(ngModel)]="field.name" 
                placeholder="key_name" 
                class="w-full bg-transparent text-sm text-zinc-200 focus:outline-none placeholder-zinc-700 font-mono"
              />
            </div>
            <div class="col-span-3">
              <select [(ngModel)]="field.type" class="w-full bg-surface text-xs text-zinc-300 rounded border border-zinc-700 px-2 py-1 focus:border-primary focus:outline-none">
                <option value="string">String</option>
                <option value="number">Number</option>
                <option value="boolean">Boolean</option>
                <option value="date">Date</option>
                <option value="list">List</option>
              </select>
            </div>
            <div class="col-span-2 flex items-center">
              <label class="inline-flex items-center cursor-pointer">
                <input type="checkbox" [(ngModel)]="field.required" class="form-checkbox h-3.5 w-3.5 text-primary rounded bg-zinc-800 border-zinc-600 focus:ring-offset-zinc-900">
              </label>
            </div>
            <div class="col-span-2 flex justify-end">
              <button (click)="removeField($index)" class="text-zinc-600 hover:text-red-400 p-1 rounded hover:bg-red-500/10 transition-colors opacity-0 group-hover:opacity-100">
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16" />
                </svg>
              </button>
            </div>
          </div>
        }

        <!-- Add Button -->
        <button (click)="addField()" class="w-full py-2 border border-dashed border-zinc-800 rounded text-zinc-600 text-xs hover:text-primary hover:border-primary/50 hover:bg-primary/5 transition-all flex items-center justify-center gap-1 mt-2">
          <svg xmlns="http://www.w3.org/2000/svg" class="h-3 w-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 4v16m8-8H4" />
          </svg>
          Add Property
        </button>
      </div>
    </div>
  `
})
export class SchemaBuilderComponent {
  schema = input.required<SchemaField[]>();
  save = output<SchemaField[]>();
  
  localSchema = signal<SchemaField[]>([]);

  constructor() {
    effect(() => {
      this.localSchema.set(JSON.parse(JSON.stringify(this.schema())));
    });
  }

  addField() {
    this.localSchema.update(s => [...s, { name: '', type: 'string', required: false }]);
  }

  removeField(index: number) {
    this.localSchema.update(s => s.filter((_, i) => i !== index));
  }

  saveSchema() {
    const validSchema = this.localSchema().filter(f => f.name.trim() !== '');
    this.save.emit(validSchema);
  }
}