# Histórico de alterações

## 0.1.0 — 08/11/2026

- Reorganizadas as integrações na aba Biblioteca, com seções para Steam e Battle.net.
- Adicionada descoberta nativa dos jogos Battle.net instalados pelo Registro e pelo banco local `product.db`, sem Playnite.
- Adicionadas sincronização manual e automática da Battle.net, com preservação de entradas manuais e proteção contra leituras incompletas.
- Adicionada abertura de jogos pelo launcher, com tentativa de início minimizado, espera com tempo limite e isolamento do launcher em relação ao grupo da sessão.

- Adicionada a opção global de forçar a resolução da sessão nos jogos Steam no Windows.
- Adicionados os perfis Unity, Unreal Engine e Source / Source 2, com detecção automática pelos arquivos instalados ou seleção manual global.
- Os argumentos são calculados na inicialização com a resolução efetiva da sessão e enviados pela Steam, sem modificar suas opções salvas.
- Jogos não reconhecidos no modo automático mantêm seu comando de inicialização; o motivo é informado no log.
- Desativada temporariamente a verificação de atualizações e removida a consulta às releases do Apollo original.
- Ajustada a versão do aplicativo, da interface web e dos pacotes para 0.1.0.
- Mantidas a sincronização da biblioteca Steam, as correções de estabilidade das sessões e a inclusão completa dos arquivos do SudoVDA.
