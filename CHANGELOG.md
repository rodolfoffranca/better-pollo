# Histórico de alterações

## 0.1.0 — 08/11/2026

- Adicionada a opção global de forçar a resolução da sessão nos jogos Steam no Windows.
- Adicionados os perfis Unity, Unreal Engine e Source / Source 2, com detecção automática pelos arquivos instalados ou seleção manual global.
- Os argumentos são calculados na inicialização com a resolução efetiva da sessão e enviados pela Steam, sem modificar suas opções salvas.
- Jogos não reconhecidos no modo automático mantêm seu comando de inicialização; o motivo é informado no log.
- Desativada temporariamente a verificação de atualizações e removida a consulta às releases do Apollo original.
- Ajustada a versão do aplicativo, da interface web e dos pacotes para 0.1.0.
- Mantidas a sincronização da biblioteca Steam, as correções de estabilidade das sessões e a inclusão completa dos arquivos do SudoVDA.
