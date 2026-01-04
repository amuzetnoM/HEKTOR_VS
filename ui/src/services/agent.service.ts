import { Injectable, inject, signal } from '@angular/core';
import { GoogleGenAI, GenerateContentResponse, ChatSession } from '@google/genai';
import { VectorDbService } from './vector-db.service';

export interface ChatMessage {
  role: 'user' | 'model' | 'tool';
  text?: string;
  isAction?: boolean; // UI badge
  toolName?: string;
  toolArgs?: any;
  toolOutput?: string;
  timestamp: number;
}

@Injectable({
  providedIn: 'root'
})
export class AgentService {
  private dbService = inject(VectorDbService);
  private ai: GoogleGenAI;
  private chatSession: ChatSession | null = null;
  
  // State
  messages = signal<ChatMessage[]>([{
    role: 'model',
    text: 'Hello! I am Hektor, your Vector Ops Agent. I can manage collections, run queries, and visualize data. How can I help?',
    timestamp: Date.now()
  }]);
  
  isThinking = signal(false);

  constructor() {
    // Initialize AI
    const apiKey = process.env['API_KEY'] || '';
    this.ai = new GoogleGenAI({ apiKey });
  }

  private initChat() {
    if (this.chatSession) return;

    this.chatSession = this.ai.chats.create({
      model: 'gemini-2.5-flash',
      config: {
        systemInstruction: `You are an expert AI Agent managing a Vector Database Studio.
        You have access to tools to manipulate the database.
        ALWAYS use tools when the user asks to perform an action (create, delete, query, add).
        If the user asks for "visualization" or "projection", ensure data exists.
        Be concise and technical but helpful.
        When you perform an action, briefly explain what you are doing.
        `,
        tools: [{
          functionDeclarations: [
            {
              name: 'create_collection',
              description: 'Create a new vector collection.',
              parameters: {
                type: 'OBJECT',
                properties: {
                  name: { type: 'STRING', description: 'Name of the collection (slug-friendly)' },
                  dimension: { type: 'NUMBER', description: 'Vector dimension (e.g. 1536, 768)' },
                  metric: { type: 'STRING', enum: ['cosine', 'euclidean', 'dot'], description: 'Distance metric' }
                },
                required: ['name']
              }
            },
            {
              name: 'delete_collection',
              description: 'Delete an existing collection.',
              parameters: {
                type: 'OBJECT',
                properties: {
                  name: { type: 'STRING', description: 'Name of the collection to delete' }
                },
                required: ['name']
              }
            },
            {
              name: 'add_documents',
              description: 'Add raw text documents to a collection. The system will handle embedding.',
              parameters: {
                type: 'OBJECT',
                properties: {
                  collectionName: { type: 'STRING' },
                  texts: { type: 'ARRAY', items: { type: 'STRING' }, description: 'List of text strings to add' }
                },
                required: ['collectionName', 'texts']
              }
            },
            {
              name: 'query_vector',
              description: 'Semantic search / query vectors.',
              parameters: {
                type: 'OBJECT',
                properties: {
                  collectionName: { type: 'STRING' },
                  queryText: { type: 'STRING' },
                  topK: { type: 'NUMBER' }
                },
                required: ['collectionName', 'queryText']
              }
            }
          ]
        }]
      }
    });
  }

  async sendMessage(text: string) {
    this.initChat();
    if (!this.chatSession) return;

    // Add user message
    this.messages.update(msgs => [...msgs, { role: 'user', text, timestamp: Date.now() }]);
    this.isThinking.set(true);

    try {
      let response = await this.chatSession.sendMessage({ message: text });
      await this.handleResponse(response);
    } catch (err) {
      console.error(err);
      this.messages.update(msgs => [...msgs, { role: 'model', text: 'Error executing request.', timestamp: Date.now() }]);
    } finally {
      this.isThinking.set(false);
    }
  }

  private async handleResponse(response: GenerateContentResponse) {
    // Check for tool calls
    const candidates = response.candidates || [];
    const content = candidates[0]?.content;
    const parts = content?.parts || [];

    // 1. If text response, show it
    const textPart = parts.find(p => p.text);
    if (textPart && textPart.text) {
      this.messages.update(msgs => [...msgs, { 
        role: 'model', 
        text: textPart.text, 
        timestamp: Date.now() 
      }]);
    }

    // 2. If tool calls, execute them
    const functionCalls = parts.filter(p => p.functionCall).map(p => p.functionCall);
    
    if (functionCalls.length > 0) {
      const toolOutputs: any[] = [];

      for (const call of functionCalls) {
        if (!call) continue;
        const fnName = call.name;
        const args = call.args;

        // Show "Action" UI
        this.messages.update(msgs => [...msgs, {
          role: 'tool',
          isAction: true,
          toolName: fnName,
          toolArgs: args,
          timestamp: Date.now()
        }]);

        try {
          let result = "Done";
          if (fnName === 'create_collection') {
            result = await this.dbService.createCollection(args['name'], args['dimension'], args['metric']);
          } else if (fnName === 'delete_collection') {
            result = await this.dbService.deleteCollection(args['name']);
          } else if (fnName === 'add_documents') {
            const docs = (args['texts'] as string[]).map(t => ({ content: t }));
            result = await this.dbService.addDocuments(args['collectionName'], docs);
          } else if (fnName === 'query_vector') {
            const results = await this.dbService.query(args['collectionName'], args['queryText'], args['topK']);
            result = JSON.stringify(results.map((r: any) => ({ id: r.id, score: r.score, content: r.content.substring(0, 50) + '...' })));
          }

          toolOutputs.push({
            functionResponse: {
              name: fnName,
              response: { result: result }
            }
          });

          // Show Output UI
           this.messages.update(msgs => [...msgs, {
            role: 'tool',
            toolName: fnName,
            toolOutput: result,
            timestamp: Date.now()
          }]);

        } catch (e: any) {
           toolOutputs.push({
            functionResponse: {
              name: fnName,
              response: { error: e.message }
            }
          });
          this.messages.update(msgs => [...msgs, {
            role: 'tool',
            toolName: fnName,
            toolOutput: `Error: ${e.message}`,
            timestamp: Date.now()
          }]);
        }
      }

      // Send tool outputs back to model to get final summary
      if (this.chatSession) {
         const finalResponse = await this.chatSession.sendMessage({
            message: toolOutputs // Pass raw tool outputs
         });
         
         const finalParts = finalResponse.candidates?.[0]?.content?.parts || [];
         const finalText = finalParts.find(p => p.text)?.text;
         if (finalText) {
             this.messages.update(msgs => [...msgs, { 
                role: 'model', 
                text: finalText, 
                timestamp: Date.now() 
            }]);
         }
      }
    }
  }
}