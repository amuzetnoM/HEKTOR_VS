import { Component, inject, ElementRef, ViewChild, signal, effect } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { AgentService, ChatMessage } from '../services/agent.service';

@Component({
  selector: 'app-chat-widget',
  standalone: true,
  imports: [CommonModule, FormsModule],
  template: `
    <!-- Floating Container -->
    <div class="fixed bottom-6 right-0 md:right-6 px-4 md:px-0 z-50 flex flex-col items-end gap-4 font-sans w-full md:w-auto pointer-events-none">
      
      <!-- Chat Window (Expanded) -->
      @if (isOpen()) {
        <div class="w-full md:w-[420px] h-[500px] md:h-[600px] max-h-[70vh] flex flex-col bg-[#09090b] border border-white/10 rounded-xl shadow-2xl overflow-hidden animate-in slide-in-from-bottom-5 fade-in duration-200 ring-1 ring-white/5 pointer-events-auto">
          
          <!-- Header -->
          <div class="h-12 border-b border-white/5 bg-zinc-900/50 backdrop-blur flex items-center justify-between px-4 shrink-0">
            <div class="flex items-center gap-2.5">
               <div class="relative">
                 <div class="w-2.5 h-2.5 rounded-full bg-indigo-500 shadow-[0_0_10px_rgba(99,102,241,0.5)]"></div>
                 @if (agent.isThinking()) {
                   <div class="absolute inset-0 w-2.5 h-2.5 rounded-full bg-indigo-500 animate-ping opacity-75"></div>
                 }
               </div>
               <span class="text-xs font-semibold text-zinc-200 tracking-wide">HEKTOR</span>
               <span class="text-[10px] px-1.5 py-0.5 rounded bg-white/5 text-zinc-500 border border-white/5 font-mono">v2.0</span>
            </div>
            <button (click)="toggle()" class="text-zinc-500 hover:text-white transition-colors p-1">
              <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" viewBox="0 0 20 20" fill="currentColor">
                <path fill-rule="evenodd" d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z" clip-rule="evenodd" />
              </svg>
            </button>
          </div>

          <!-- Messages -->
          <div #scrollContainer class="flex-1 overflow-y-auto p-4 space-y-5 bg-[#09090b]">
            @for (msg of agent.messages(); track msg.timestamp) {
              
              <!-- User Message -->
              @if (msg.role === 'user') {
                <div class="flex flex-col items-end gap-1">
                  <div class="flex justify-end pl-10">
                    <div class="bg-indigo-600/10 border border-indigo-500/20 text-indigo-100 rounded-2xl rounded-tr-sm px-4 py-2.5 text-sm leading-relaxed shadow-sm">
                      {{ msg.text }}
                    </div>
                  </div>
                  <!-- RAG Tools Debug Info -->
                  @if (msg.retrievedTools?.length) {
                    <div class="flex items-center gap-1.5 text-[9px] text-zinc-600 pr-1 animate-in fade-in slide-in-from-right-2 duration-500">
                       <svg xmlns="http://www.w3.org/2000/svg" class="h-3 w-3" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z" /></svg>
                       <span>Loaded Tools: {{ msg.retrievedTools?.join(', ') }}</span>
                    </div>
                  }
                </div>
              }

              <!-- Model Message -->
              @if (msg.role === 'model') {
                <div class="flex flex-col gap-2 pr-4 animate-in fade-in duration-300">
                  <div class="flex items-center gap-2">
                     <span class="text-[10px] font-bold text-indigo-400 uppercase tracking-wider">Assistant</span>
                  </div>
                  <div class="text-zinc-300 text-sm leading-relaxed font-light whitespace-pre-wrap">
                    {{ msg.text }}
                  </div>
                </div>
              }

              <!-- Action Badge -->
              @if (msg.isAction) {
                 <div class="py-2 px-3 border border-indigo-500/20 bg-indigo-500/5 rounded-lg flex items-center justify-between gap-3 mx-4">
                    <div class="flex items-center gap-2.5">
                       <div class="w-1.5 h-1.5 bg-indigo-500 rounded-full animate-pulse"></div>
                       <div class="font-mono text-[10px] text-indigo-300 uppercase tracking-wider">EXEC_TOOL :: {{ msg.toolName }}</div>
                    </div>
                 </div>
              }

              <!-- Tool Output -->
              @if (msg.role === 'tool' && msg.toolOutput) {
                 <div class="mx-4 bg-black/40 rounded-lg border border-white/5 p-3 overflow-hidden">
                    <div class="text-[9px] text-zinc-600 font-mono mb-1.5 uppercase flex items-center gap-2">
                       <svg xmlns="http://www.w3.org/2000/svg" class="h-3 w-3" viewBox="0 0 20 20" fill="currentColor"><path fill-rule="evenodd" d="M12.316 3.051a1 1 0 01.633 1.265l-4 12a1 1 0 11-1.898-.632l4-12a1 1 0 011.265-.633zM5.707 6.293a1 1 0 010 1.414L3.414 10l2.293 2.293a1 1 0 11-1.414 1.414l-3-3a1 1 0 010-1.414l3-3a1 1 0 011.414 0zm8.586 0a1 1 0 011.414 0l3 3a1 1 0 010 1.414l-3 3a1 1 0 11-1.414-1.414L16.586 10l-2.293-2.293a1 1 0 010-1.414z" clip-rule="evenodd" /></svg>
                       Output Stream
                    </div>
                    <div class="font-mono text-[10px] text-emerald-500/90 whitespace-pre-wrap break-all leading-relaxed max-h-32 overflow-y-auto custom-scrollbar">
                      {{ msg.toolOutput | slice:0:300 }}{{ msg.toolOutput.length > 300 ? '...' : '' }}
                    </div>
                 </div>
              }
            }

            @if (agent.isThinking()) {
               <div class="flex items-center gap-2 text-zinc-500 text-xs px-1">
                  <div class="flex space-x-1">
                    <div class="w-1 h-1 bg-zinc-500 rounded-full animate-bounce" style="animation-delay: 0ms"></div>
                    <div class="w-1 h-1 bg-zinc-500 rounded-full animate-bounce" style="animation-delay: 150ms"></div>
                    <div class="w-1 h-1 bg-zinc-500 rounded-full animate-bounce" style="animation-delay: 300ms"></div>
                  </div>
                  <span class="font-mono text-[10px] uppercase tracking-wider opacity-70">Processing</span>
               </div>
            }
          </div>

          <!-- Quick Actions -->
          <div class="px-4 pb-2 pt-2 bg-zinc-900/30 backdrop-blur-sm border-t border-white/5 flex gap-2 overflow-x-auto custom-scrollbar no-scrollbar">
            @for (action of quickActions; track action.label) {
               <button 
                  (click)="executeAction(action.prompt)"
                  [disabled]="agent.isThinking()"
                  class="whitespace-nowrap px-3 py-1.5 rounded-full bg-zinc-800 hover:bg-indigo-500/20 hover:text-indigo-300 hover:border-indigo-500/30 border border-white/5 text-[10px] font-medium text-zinc-400 transition-all flex-shrink-0 disabled:opacity-50 disabled:cursor-not-allowed">
                  {{ action.label }}
               </button>
            }
          </div>

          <!-- Input Area -->
          <div class="p-4 bg-zinc-900/30 backdrop-blur-sm pt-2">
            <div class="relative group">
              <input 
                #chatInput
                type="text" 
                [(ngModel)]="inputText"
                (keydown.enter)="send()"
                placeholder="Ask to create a collection..."
                class="w-full bg-[#09090b] border border-white/10 rounded-lg pl-4 pr-10 py-3 text-sm text-zinc-200 focus:outline-none focus:border-indigo-500/50 focus:ring-1 focus:ring-indigo-500/20 placeholder-zinc-700 transition-all shadow-inner"
                [disabled]="agent.isThinking()"
              />
              <button 
                (click)="send()"
                class="absolute right-2 top-2 p-1.5 rounded-md text-zinc-500 hover:text-indigo-400 hover:bg-white/5 transition-all disabled:opacity-50">
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" viewBox="0 0 20 20" fill="currentColor">
                  <path d="M10.894 2.553a1 1 0 00-1.788 0l-7 14a1 1 0 001.169 1.409l5-1.429A1 1 0 009 15.571V11a1 1 0 112 0v4.571a1 1 0 00.725.962l5 1.428a1 1 0 001.17-1.408l-7-14z" />
                </svg>
              </button>
            </div>
            <div class="text-[10px] text-zinc-700 text-center mt-2 font-mono">
               Powered by Google Gemini 2.5 Flash
            </div>
          </div>
        </div>
      }

      <!-- Toggle Button (FAB) -->
      <button 
        (click)="toggle()"
        class="group relative h-14 w-14 rounded-full bg-indigo-600 hover:bg-indigo-500 text-white shadow-[0_0_20px_rgba(79,70,229,0.4)] flex items-center justify-center transition-all duration-300 hover:scale-105 active:scale-95 border border-indigo-400/30 pointer-events-auto">
        
        <!-- Icon -->
        @if (!isOpen()) {
           <svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6 transition-transform group-hover:rotate-12" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 10h.01M12 10h.01M16 10h.01M9 16H5a2 2 0 01-2-2V6a2 2 0 012-2h14a2 2 0 012 2v8a2 2 0 01-2 2h-5l-5 5v-5z" />
          </svg>
        } @else {
           <svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6 rotate-90" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7" />
          </svg>
        }

        <!-- Notification Badge -->
        @if (!isOpen() && hasUnread()) {
          <span class="absolute top-0 right-0 h-4 w-4 rounded-full bg-red-500 border-2 border-[#09090b]"></span>
        }
      </button>
    </div>
  `,
  styles: [`
    .custom-scrollbar::-webkit-scrollbar { width: 4px; height: 4px; }
    .custom-scrollbar::-webkit-scrollbar-thumb { background: #3f3f46; border-radius: 2px; }
    .no-scrollbar::-webkit-scrollbar { display: none; }
    .no-scrollbar { -ms-overflow-style: none; scrollbar-width: none; }
  `]
})
export class ChatWidgetComponent {
  agent = inject(AgentService);
  inputText = '';
  isOpen = signal(false);
  hasUnread = signal(false);
  
  @ViewChild('scrollContainer') scrollContainer!: ElementRef;
  @ViewChild('chatInput') chatInput!: ElementRef;

  quickActions = [
    { label: 'Create Collection', prompt: 'Create a new vector collection for me.' },
    { label: 'System Status', prompt: 'What is the current system status and document count?' },
    { label: 'List Tools', prompt: 'What tools and functions do you have access to?' },
    { label: 'Import Data', prompt: 'How do I import data into the database?' }
  ];

  constructor() {
    effect(() => {
      const msgs = this.agent.messages();
      if (!this.isOpen() && msgs.length > 1) { // Assuming initial greeting is read
         this.hasUnread.set(true);
      }
      setTimeout(() => this.scrollToBottom(), 50);
    });
  }

  toggle() {
    this.isOpen.update(v => !v);
    if (this.isOpen()) {
      this.hasUnread.set(false);
      setTimeout(() => this.chatInput?.nativeElement?.focus(), 100);
      setTimeout(() => this.scrollToBottom(), 100);
    }
  }

  executeAction(prompt: string) {
    this.agent.sendMessage(prompt);
  }

  send() {
    if (!this.inputText.trim()) return;
    this.agent.sendMessage(this.inputText);
    this.inputText = '';
  }

  scrollToBottom() {
    if (this.scrollContainer) {
      this.scrollContainer.nativeElement.scrollTop = this.scrollContainer.nativeElement.scrollHeight;
    }
  }
}