
import { bootstrapApplication } from '@angular/platform-browser';
import { provideZonelessChangeDetection } from '@angular/core';
import { provideHttpClient, withFetch } from '@angular/common/http';
import { AppComponent } from './src/app.component';

bootstrapApplication(AppComponent, {
  providers: [
    provideZonelessChangeDetection(),
    provideHttpClient(withFetch())
  ]
}).catch(err => console.error(err));

// AI Studio always uses an `index.tsx` file for all project types.
