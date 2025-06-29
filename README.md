# mod_muse-ai

**An Apache module for AI-powered content generation**

> ⚠️ **WORK IN PROGRESS** - This project is actively being developed and should be considered experimental. There will be bugs, there are certainly better ways to implement many features, and the codebase is evolving rapidly. Contributions, feedback, and improvements are not just welcome - they're essential!

> 🌊 **Vibe Coded with [Windsurf](https://windsurf.com/)** - This entire project was developed using Windsurf's revolutionary AI Flow paradigm, showcasing the power of human-AI collaboration in creating production-ready software.

## What is mod_muse-ai?

mod_muse-ai is an experimental Apache module that integrates AI content generation into your web server. It allows you to create `.ai` files containing prompts that get processed by AI services to generate web content.

The module can:
- Process `.ai` files containing text prompts
- Generate web pages using AI services (OpenAI API, Ollama, etc.)
- Stream AI responses back to visitors
- Work with OpenAI-compatible APIs

## Why mod_muse-ai?

This project explores integrating AI content generation directly into Apache, eliminating the need for separate backend services. It's a simple approach to AI-powered web content that works with existing Apache configurations.

**Important:** This is experimental software under active development. Expect bugs, rough edges, and frequent changes. The code likely contains suboptimal implementations that could be improved - and that's where you come in!

## Current Features

- **`.ai` file processing** - Create files with prompts that generate web content
- **AI service integration** - Works with OpenAI API and Ollama
- **Streaming responses** - Content is delivered as it's generated
- **System prompts** - Define consistent styling and layout templates
- **Dual URL support** - Access content via `/page.ai` or `/page` URLs

## Getting Started

See [HOWTO.md](HOWTO.md) for installation and configuration instructions.

### Basic Example
```bash
# Create an AI-powered page
echo "Write a simple HTML page with a welcome message" > /var/www/html/welcome.ai

# Visit http://localhost/welcome.ai
# The AI will generate content based on your prompt
```

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────┐    ┌─────────────────┐
│   Apache HTTP   │    │  mod_muse-ai │    │   AI Backend    │
│     Server      │◄──►│    Module    │◄──►│ (Ollama/OpenAI) │
└─────────────────┘    └──────────────┘    └─────────────────┘
         ▲                       │
         │                       ▼
    ┌─────────┐           ┌─────────────┐
    │ Web     │           │ Prompt      │
    │ Browser │           │ Templates   │
    └─────────┘           └─────────────┘
```

## Development Status

mod_muse-ai is experimental software under active development. Current status:

**Working:**
- Basic `.ai` file processing
- AI service integration (OpenAI API, Ollama)
- Streaming responses
- System prompt templates
- **AI-powered translation system (46 languages)**
- **URL localization with language prefixes**
- **Multi-method language detection (URL, query, cookie, headers)**

**In Development:**
- Advanced caching and performance features
- Enhanced error handling
- Production hardening

**Not Ready For:**
- Production use
- High-traffic websites
- Mission-critical applications

## Contributing

**I need your help!** This project is a work in progress, and there are definitely better ways to implement many of the features. Whether you're an Apache expert, C developer, AI enthusiast, or just someone who found a bug - your contributions are invaluable.

### Ways to Contribute:
- 🐛 **Report bugs** - Found something broken? [Open an issue](https://github.com/kekePower/mod_muse-ai/issues)
- 💡 **Suggest improvements** - See a better way to do something? We want to hear it!
- 🔧 **Submit code** - Fix bugs, add features, or refactor existing code
- 📚 **Improve documentation** - Help others understand and use the project
- 🧪 **Test and provide feedback** - Try it out and share your experiences
- 🎨 **Share use cases** - Show us what you're building with mod_muse-ai

### No Contribution is Too Small
Whether it's fixing a typo, suggesting a better algorithm, or rewriting entire modules - all contributions are appreciated. This is a learning project, and we're all figuring it out together.

Join the conversation on [GitHub discussions](https://github.com/kekePower/mod_muse-ai/discussions) for project updates, questions, and community discussion.

## 🔗 Powered By

- **[MuseWeb](https://github.com/kekePower/museweb)** – The original AI web framework that inspired this project
- **[Apache HTTP Server](https://httpd.apache.org/)** – The world's most popular web server
- **[OpenAI API](https://openai.com/api/)** & **[Ollama](https://ollama.ai/)** – The AI engines that make the magic happen

---

## 📄 License

mod_muse-ai is open source software released under the **Apache License 2.0**. This means you can use it freely in both personal and commercial projects, modify it to fit your needs, and even redistribute it. The only requirement is to include the original license notice.