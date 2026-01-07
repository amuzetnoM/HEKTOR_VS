import { Injectable, inject, signal } from '@angular/core';
import { GoogleGenAI, GenerateContentResponse, Chat, Content } from '@google/genai';
import { VectorDbService } from './vector-db.service';

export interface ChatMessage {
  role: 'user' | 'model' | 'tool';
  text?: string;
  isAction?: boolean; // UI badge
  toolName?: string;
  toolArgs?: any;
  toolOutput?: string;
  timestamp: number;
  retrievedTools?: string[]; // To show which tools were pulled from DB
}

@Injectable({
  providedIn: 'root'
})
export class AgentService {
  private dbService = inject(VectorDbService);
  private ai: GoogleGenAI;
  
  // We hold the session manually to allow model-swapping (different tools per turn)
  private chatHistory: Content[] = [];
  private chatSession: Chat | null = null;
  
  // State
  messages = signal<ChatMessage[]>([{
    role: 'model',
    text: 'Welcome to your database. \nI am connected to the Dynamic Tool Registry.',
    timestamp: Date.now()
  }]);
  
  isThinking = signal(false);

  constructor() {
    const apiKey = process.env['API_KEY'] || '';
    this.ai = new GoogleGenAI({ apiKey });
  }

  async sendMessage(text: string) {
    this.isThinking.set(true);

    // Add user message
    this.messages.update(msgs => [...msgs, { role: 'user', text, timestamp: Date.now() }]);

    try {
      // 1. Dynamic Tool Retrieval (RAG for Tools)
      //    We search the 'system_tools' collection for tools relevant to this specific prompt.
      const relevantTools = await this.dbService.retrieveTools(text, 3);
      
      const toolNames = relevantTools.map(t => t.name);
      // Update the last message to show what we found (Optional UI enhancement)
      this.messages.update(msgs => {
        const last = msgs[msgs.length - 1];
        return [...msgs.slice(0, -1), { ...last, retrievedTools: toolNames }];
      });

      // 2. Instantiate Model with JUST the relevant tools
      //    This keeps the context window clean and focused.
      this.chatSession = this.ai.chats.create({
        model: 'gemini-2.5-flash',
        history: this.chatHistory, // Pass previous history
        config: {
          systemInstruction: `You are an expert AI Agent managing a Vector Database Studio.
          You have access to a Dynamic Tool Registry. 
          For this turn, the system has retrieved the following tools for you: ${toolNames.join(', ')}.
          ALWAYS use these tools if the user asks to perform an action.
          If no relevant tools are present for the request, apologize and explain you don't have that capability loaded.
          `,
          tools: relevantTools.length > 0 ? [{ functionDeclarations: relevantTools }] : []
        }
      });

      // 3. Send Message
      let response = await this.chatSession.sendMessage({ message: text });
      
      // Update history tracking
      this.chatHistory = await this.chatSession.getHistory();

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
        // In recent SDKs, args might be Record<string, any> or unknown. Casting to any for flexibility here.
        const args = call.args as any;

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
          // We can execute these because the service methods are generic
          if (fnName === 'create_collection') {
            result = await this.dbService.createCollection(args['name'], args['dimension'], args['metric']);
          } else if (fnName === 'delete_collection') {
            result = await this.dbService.deleteCollection(args['name']);
          } else if (fnName === 'add_documents') {
            const docs = (args['texts'] as string[]).map(t => ({ content: t }));
            result = await this.dbService.addDocuments(args['collectionName'], docs);
          } else if (fnName === 'query_vector') {
            const results = await this.dbService.query(args['collectionName'], {
              query: args['queryText'],
              topK: args['topK'],
              minScore: 0.6
            });
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
      // We must reuse the same session to complete the function call loop
      if (this.chatSession) {
         const finalResponse = await this.chatSession.sendMessage({
            message: toolOutputs // Pass raw tool outputs
         });
         
         // Sync History again
         this.chatHistory = await this.chatSession.getHistory();

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